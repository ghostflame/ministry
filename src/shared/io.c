/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* io.c - handles network i/o and buffered writes                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"

IO_CTL *_io;


__attribute__((hot)) int io_read_data( SOCK *s )
{
	int i;

	if( !( i = recv( s->fd, s->in->buf + s->in->len, s->in->sz - ( s->in->len + 2 ), MSG_DONTWAIT ) ) )
	{
		if( s->flags & IO_CLOSE_EMPTY )
		{
			// that would be the fin, then
			debug( "Received a FIN, perhaps, from %s", s->name );
			flagf_add( s, IO_CLOSE );
		}
		return 0;
	}
	else if( i < 0 )
	{
		if( errno != EAGAIN
		 && errno != EWOULDBLOCK )
		{
			err( "Recv error for host %s -- %s",
				s->name, Err );
			flagf_add( s, IO_CLOSE );
			return i;
		}
		return 0;
	}

	// got some data then
	s->in->len += i;

	//debug( "Received %d bytes on socket %d/%s", i, s->fd, s->name );

	return i;
}




// we have to pass an offset in because
// we cannot modify the buffer, else later
// threads writing the same buffer will
// perceive it to be empty
int io_write_data( SOCK *s, int off )
{
	int rv, len, wr, tries, sent;
	struct pollfd p;
	char *ptr;
	IOBUF *b;

	p.fd     = s->fd;
	p.events = POLLOUT;
	tries    = 3;
	sent     = 0;
	b        = s->out;
	ptr      = b->buf + off;
	len      = b->len - off;

	while( len > 0 )
	{
		if( ( rv = poll( &p, 1, 20 ) ) < 0 )
		{
			if( errno == EINTR )
				continue;

			warn( "Poll error writing to host %s -- %s",
				s->name, Err );
			flagf_add( s, IO_CLOSE );
			return sent;
		}

		if( !rv )
		{
			// wait another 20msec and try again
			if( tries-- > 0 )
				continue;

			// we cannot write just yet
			return sent;
		}

		if( ( wr = send( s->fd, ptr, len, MSG_NOSIGNAL ) ) < 0 )
		{
			warn( "Error writing to host %s -- %s",
				s->name, Err );
			flagf_add( s, IO_CLOSE );
			return sent;
		}

		len  -= wr;
		ptr  += wr;
		sent += wr;
	}

	// what we wrote - adds to offset
	return sent;
}


int io_connected( SOCK *s )
{
	socklen_t len;
	int err;

	if( s->fd < 0 )
	{
		debug( "Not connected, aborting check." );
		return -1;
	}

	err = 0;
	len = sizeof( int );

	if( getsockopt( s->fd, SOL_SOCKET, SO_ERROR, &err, &len ) < 0 )
	{
		warn( "Could not assess target socket state: %s", Err );
		close( s->fd );
		s->fd = -1;
		return -1;
	}

	if( err != 0 )
	{
		warn( "I/O socket errored: %s", strerror( err ) );
		close( s->fd );
		s->fd = -1;
		return -1;
	}

	return 0;
}



int io_connect( SOCK *s )
{
	int opt = 1;

	if( s->fd != -1 )
	{
		warn( "Net connect called on connected socket - disconnecting." );
		shutdown( s->fd, SHUT_RDWR );
		close( s->fd );
		s->fd = -1;
	}

	if( ( s->fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
	{
		err( "Unable to make tcp socket for %s -- %s", s->name, Err );
		return -1;
	}

	if( setsockopt( s->fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof( int ) ) )
	{
		err( "Unable to set keepalive on socket for %s -- %s",
			s->name, Err );
		close( s->fd );
		s->fd = -1;
		return -1;
	}

	if( connect( s->fd, (struct sockaddr *) &(s->peer), sizeof( struct sockaddr_in ) ) < 0 )
	{
		err( "Unable to connect to %s:%hu -- %s",
			inet_ntoa( s->peer.sin_addr ), ntohs( s->peer.sin_port ),
			Err );
		close( s->fd );
		s->fd = -1;
		return -1;
	}

	info( "Connected (%d) to remote host %s:%hu.",
		s->fd, inet_ntoa( s->peer.sin_addr ),
		ntohs( s->peer.sin_port ) );

	s->flags = 0;

	return s->fd;
}



void io_disconnect( SOCK *s )
{
	if( s->fd < 0 )
		return;

	if( shutdown( s->fd, SHUT_RDWR ) )
		err( "Shutdown error on connection with %s -- %s",
			s->name, Err );

	close( s->fd );
	s->fd = -1;
}


SOCK *io_make_sock( int32_t insz, int32_t outsz, struct sockaddr_in *peer )
{
	SOCK *s;

	if( !( s = (SOCK *) allocz( sizeof( SOCK ) ) ) )
		fatal( "Could not allocate new nsock." );

	if( !s->name )
		s->name = perm_str( 32 );

	snprintf( s->name, 32, "%s:%hu", inet_ntoa( peer->sin_addr ),
		ntohs( peer->sin_port ) );

	// copy the peer contents
	s->peer = *peer;

	if( insz )
		s->in = mem_new_iobuf( insz );

	if( outsz )
		s->out = mem_new_iobuf( outsz );

	// no socket yet
	s->fd = -1;

	return s;
}



// buffers

void io_buf_decr( IOBUF *buf )
{
	int refs;

	lock_buf( buf );
	refs = --(buf->refs);
	unlock_buf( buf );

	if( refs <= 0 )
		mem_free_iobuf( &buf );
}


IOBUF *io_buf_fetch( TGT *t )
{
	IOBP *bp = NULL;
	IOBUF *b = NULL;

	lock_target( t );

	if( t->tail )
	{
		// take one off the end
		bp = t->tail;
		t->tail = bp->prev;
		t->curr--;

		// was that the last one?
		if( t->tail )
			t->tail->next = NULL;
		else
			t->head = NULL;
	}

	unlock_target( t );

	if( bp )
	{
		b = bp->buf;
		mem_free_iobp( &bp );

		//tgdebug( "Grabbed buffer %p, %d remaining", b, t->curr );
	}

	return b;
}



void __io_buf_post_one( TGT *t, IOBUF *b )
{
	IOBP *bp;

	// dump buffers on anything at max
	if( t->curr >= t->max )
	{
		tgdebug( "Hit max waiting %d.", t->max );
		io_buf_decr( b );
		return;
	}

	bp = mem_new_iobp( );
	bp->buf = b;

	lock_target( t );

	if( t->tail )
	{
		bp->next = t->head;
		t->head->prev = bp;
		t->head  = bp;
	}
	else
	{
		t->head = bp;
		t->tail = bp;
	}

	t->curr++;
	//tgdebug( "Target now has %d bufs.", t->curr );

	unlock_target( t );
}

void io_buf_post_one( TGT *t, IOBUF *b )
{
	if( b->refs <= 0 )
		b->refs = 1;

	__io_buf_post_one( t, b );
}


// post buffer to list
void io_buf_post( TGTL *l, IOBUF *buf )
{
	TGT *p;

	if( !buf || !l )
	{
		warn( "io_buf_post: nulls: %p, %p", l, buf );
		return;
	}

	if( !buf->len )
	{
		warn( "io_buf_post: empty buffer." );
		io_buf_decr( buf );
		return;
	}

	// set the refs if told
	if( buf->refs <= 0 )
		buf->refs = l->count;

	// and send to each one
	for( p = l->targets; p; p = p->next )
		__io_buf_post_one( p, buf );
}


void io_buf_next( TGT *t )
{
	IOBUF *b;

	t->curr_off  = 0;
	t->curr_len  = 0;
	t->sock->out = NULL;

	if( !( b = io_buf_fetch( t ) ) )
		return;

	t->sock->out = b;
	t->curr_len  = b->len;
}



// io senders


// to the terminal|pipe
int64_t io_send_stdout( TGT *t )
{
	SOCK *s = t->sock;
	int64_t f = 0;

	if( !s->out )
		io_buf_next( t );

	// keep writing while there's something to send
	while( s->out )
	{
		t->curr_off += write( s->fd, s->out->buf + t->curr_off, s->out->len - t->curr_off );
		f++;

		if( t->curr_off >= t->curr_len )
		{
			io_buf_decr( s->out );
			io_buf_next( t );
		}
	}

	return f;
}


// to a network - needs to check connection
int64_t io_send_net_tcp( TGT *t )
{
	SOCK *s = t->sock;
	int64_t f = 0;

	// are we waiting to reconnect?
	if( t->rc_count > 0 )
	{
		t->rc_count--;
		return 0;
	}

	// are we connected?
	if( io_connected( s ) < 0
	 && io_connect( s ) < 0 )
	{
		t->rc_count = t->rc_limit;
		return 0;
	}

	if( !s->out )
		io_buf_next( t );

	while( s->out )
	{
		t->curr_off += io_write_data( s, t->curr_off );
		f++;

		// any problems?
		if( flagf_has( s, IO_CLOSE ) )
		{
			tgdebug( "Disconnecting." );
			io_disconnect( s );
			flagf_rmv( s, IO_CLOSE );
			// try again later
			break;
		}

		// did we send it all?
		if( t->curr_off >= t->curr_len )
		{
			// get a new one
			io_buf_decr( s->out );
			io_buf_next( t );
		}
		else
		{
			// try again later
			break;
		}
	}

	return f;
}


// not yet
int64_t io_send_net_udp( TGT *t )
{
	return -1;
}



// not yet
int64_t io_send_file( TGT *t )
{
	return -1;
}




// processing control


int io_init( void )
{
	IO_CTL *i = _io;
	int k;

	// allocate and init our locks
	i->lock_size = 1 << i->lock_bits;
	i->lock_mask = i->lock_size - 1;

	i->locks = (io_lock_t *) allocz( i->lock_size * sizeof( io_lock_t ) );

	for( k = 0; k < i->lock_size; k++ )
	{
		io_lock_init( i->locks[k] );
	}

	io_lock_init( i->idlock );

	return 0;
}


void io_stop( void )
{
	IO_CTL *i = _io;
	int k;

	for( k = 0; k < i->lock_size; k++ )
	{
		io_lock_destroy( i->locks[k] );
	}
}




IO_CTL *io_config_defaults( void )
{
	_io = (IO_CTL *) allocz( sizeof( IO_CTL ) );
	_io->lock_bits = IO_BLOCK_BITS;
	_io->send_usec = IO_SEND_USEC;
	_io->rc_msec   = IO_RECONN_DELAY;

	return _io;
}


int io_config_line( AVP *av )
{
	int64_t i;

	if( attIs( "sendUsec" ) )
	{
		if( parse_number( av->val, &i, NULL ) == NUM_INVALID )
		{
			err( "Invalid send usec value: %s", av->val );
			return -1;
		}

		if( i < 500 || i > 5000000 )
			warn( "Recommended send usec values are 500 <= x <= 5000000." );

		_io->send_usec = (int32_t) i;
	}
	else if( attIs( "sendMsec" ) )
	{
		if( parse_number( av->val, &i, NULL ) == NUM_INVALID )
		{
			err( "Invalid send msec value: %s", av->val );
			return -1;
		}
		if( i < 1 || i > 5000 )
			warn( "Recommended send msec values are 1 <= x <= 5000." );

		_io->send_usec = (int32_t) ( 1000 * i );
	}
	else if( attIs( "reconnectMsec" ) || attIs( "reconnMsec" ) )
	{
		if( parse_number( av->val, &i, NULL ) == NUM_INVALID )
		{
			err( "Invalid reconnect msec value: %s", av->val );
			return -1;
		}

		if( i < 20 || i > 50000 )
			warn( "Recommended reconnect msec values are 20 <= x <= 50000." );

		_io->rc_msec = (int32_t) i;
	}
	else if( attIs( "bufLockBits" ) )
	{
		if( parse_number( av->val, &i, NULL ) == NUM_INVALID )
		{
			err( "Invalid buf lock bits value: %s", av->val );
			return -1;
		}

		if( i < 3 || i > 12 )
			warn( "Recommended buf lock bits values are 3 <= x <= 12." );

		_io->rc_msec = (uint64_t) i;
	}
	else
		return -1;

	return 0;
}

