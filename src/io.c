/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* io.c - handles network i/o and buffered writes                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"


// does not have it's own config, runs off mem.c config


__attribute__((hot)) int io_read_data( NSOCK *s )
{
	int i;

	if( !( i = recv( s->sock, s->in->buf + s->in->len, s->in->sz - ( s->in->len + 2 ), MSG_DONTWAIT ) ) )
	{
		if( s->flags & HOST_CLOSE_EMPTY )
		{
			// that would be the fin, then
			debug( "Received a FIN, perhaps, from %s", s->name );
			s->flags |= HOST_CLOSE;
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
			s->flags |= HOST_CLOSE;
			return i;
		}
		return 0;
	}

	// got some data then
	s->in->len += i;

	//debug( "Received %d bytes on socket %d/%s", i, s->sock, s->name );

	return i;
}




// we have to pass an offset in because
// we cannot modify the buffer, else later
// threads writing the same buffer will
// perceive it to be empty
int io_write_data( NSOCK *s, int off )
{
	int rv, len, wr, tries, sent;
	struct pollfd p;
	char *ptr;
	IOBUF *b;

	p.fd     = s->sock;
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
			{
				debug_io( "Poll call to %s was interrupted.", s->name );
				continue;
			}

			warn( "Poll error writing to host %s -- %s",
				s->name, Err );
			s->flags |= HOST_CLOSE;
			return sent;
		}

		if( !rv )
		{
			// wait another 20msec and try again
			if( tries-- > 0 )
				continue;

			// we cannot write just yet
			debug_io( "Poll timed out try to write to %s", s->name );
			return sent;
		}

		if( ( wr = send( s->sock, ptr, len, MSG_NOSIGNAL ) ) < 0 )
		{
			warn( "Error writing to host %s -- %s",
				s->name, Err );
			s->flags |= HOST_CLOSE;
			return sent;
		}

		len  -= wr;
		ptr  += wr;
		sent += wr;
	}

	// what we wrote - adds to offset
	return sent;
}


int io_connected( NSOCK *s )
{
	socklen_t len;
	int err;

	if( s->sock < 0 )
	{
		debug_io( "Socket is -1, so not connected." );
		return -1;
	}

	err = 0;
	len = sizeof( int );
	
	if( getsockopt( s->sock, SOL_SOCKET, SO_ERROR, &err, &len ) < 0 )
	{
		warn( "Could not assess target socket state: %s", Err );
		close( s->sock );
		s->sock = -1;
		return -1;
	}

	if( err != 0 )
	{
		warn( "Target socket errored: %s", strerror( err ) );
		close( s->sock );
		s->sock = -1;
		return -1;
	}

	return 0;
}



int io_connect( TARGET *t )
{
	NSOCK *s = t->sock;
	int opt = 1;

	if( s->sock != -1 )
	{
		warn( "Net connect called on connected socket - disconnecting." );
		shutdown( s->sock, SHUT_RDWR );
		close( s->sock );
		s->sock = -1;
	}

	if( ( s->sock = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
	{
		err( "Unable to make tcp socket for %s:%hu -- %s",
			t->host, t->port, Err );
		return -1;
	}

	if( setsockopt( s->sock, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof( int ) ) )
	{
		err( "Unable to set keepalive on socket for %s:%hu -- %s",
			t->host, t->port, Err );
		close( s->sock );
		s->sock = -1;
		return -1;
	}

	if( connect( s->sock, (struct sockaddr *) &(s->peer), sizeof( struct sockaddr_in ) ) < 0 )
	{
//		err( "Unable to connect to %s:%hu -- %s",
//			inet_ntoa( s->peer.sin_addr ), ntohs( s->peer.sin_port ),
//			Err );
		close( s->sock );
		s->sock = -1;
		return -1;
	}

	info( "Connected (%d) to remote host %s:%hu.",
		s->sock, inet_ntoa( s->peer.sin_addr ),
		ntohs( s->peer.sin_port ) );

	s->flags = 0;

	return s->sock;
}




void io_decr_buf( IOBUF *buf )
{
	int free_it = 0;

	pthread_mutex_lock(   &(ctl->locks->bufref) );
	if( ! --(buf->refs) )
		free_it = 1;
	pthread_mutex_unlock( &(ctl->locks->bufref) );

	// only one of them should do this
	if( free_it )
		mem_free_buf( &buf );
}


// add a buffer under lock
void io_post_buffer( TGTIO *t, IOBUF *buf )
{
	IOLIST *l;

	// create the iolist structure
	// and hand it off into the queue
	l = mem_new_iolist( );
	l->buf = buf;

	// and attach it
	lock_tgtio( t );

	if( !t->end )
		t->end = t->head = l;
	else
	{
		// make the doubly-linked list
		l->next       = t->head;
		t->head->prev = l;
		t->head       = l;
	}

	t->bufs++;

	unlock_tgtio( t );
}


// retrive a buffer under lock
IOBUF *io_fetch_buffer( TGTIO *t )
{
	IOLIST *l = NULL;
	IOBUF *b = NULL;

	lock_tgtio( t );

	if( t->end )
	{
		// take one off the end
		l = t->end;
		t->end = l->prev;
		t->bufs--;

		// was that the last one?
		if( t->end )
			t->end->next = NULL;
		else
			t->head = NULL;
	}

	unlock_tgtio( t );

	if( l )
	{
		b = l->buf;
		mem_free_iolist( &l );
	}

	return b;
}


void io_grab_buffer( TARGET *t )
{
	IOBUF *b;

	t->curr_off  = 0;
	t->curr_len  = 0;
	t->sock->out = NULL;

	if( !( b = io_fetch_buffer( t->iolist ) ) )
		return;

	t->sock->out = b;
	t->curr_len  = b->len;

	debug_io( "Target %s:%hu has buffer %p (%d)",
		t->host, t->port, b, b->len );
}


// a much simpler loop
int64_t io_send_stdout( TARGET *t )
{
	NSOCK *s = t->sock;
	int64_t f = 0;

	// look for an outgoing buffer
	if( !s->out )
		io_grab_buffer( t );

	// keep writing while there's something to send
	while( s->out )
	{
		t->curr_off += write( s->sock, s->out->buf + t->curr_off, s->out->len - t->curr_off );
		f++;

		if( t->curr_off >= t->curr_len )
		{
			io_decr_buf( s->out );
			io_grab_buffer( t );
		}
	}

	return f;
}



int64_t io_send_net( TARGET *t )
{
	NSOCK *s = t->sock;
	int64_t f = 0;

	// are we waiting to reconnect?
	if( t->countdown > 0 )
	{
		t->countdown--;
		return 0;
	}

	// keep trying if we are not connected
	if( io_connected( t->sock ) < 0
	 && io_connect( t ) < 0 )
	{
		// don't try to reconnect at once
		// sleep a few seconds
		t->countdown = t->reconn_ct;
		return 0;
	}

	// right, were we working on anything?
	if( !s->out )
		io_grab_buffer( t );

	// while there's something to send
	while( s->out )
	{
		// try to send the out buffer
		t->curr_off += io_write_data( s, t->curr_off );
		f++;

		// did we have problems?
		if( s->flags & HOST_CLOSE )
		{
			debug_io( "Disconnecting from target %s:%hu",
				t->host, t->port );
			tcp_disconnect( &(s->sock), "send target" );
			s->flags &= ~HOST_CLOSE;
			// try again later
			break;
		}

		// did we sent it all?
		if( t->curr_off >= t->curr_len )
		{
			// drop that buffer and get a new one
			io_decr_buf( s->out );
			io_grab_buffer( t );
		}
		else
		{
			debug_io( "Target %s:%hu finishing with %d bytes to go.",
				t->host, t->port, t->curr_len - t->curr_off );
			// try again later
			break;
		}
	}

#ifdef DEBUG_IO
	if( f > 0 )
		debug_io( "Made %d writes to %s:%hu", f, t->host, t->port );
#endif

	return f;
}




void *io_loop( void *arg )
{
	struct sockaddr_in sa;
	int64_t fires = 0;
	TARGET *d;
	THRD *t;

	t = (THRD *) arg;
	d = (TARGET *) t->arg;

	// find out where we are connecting to
	if( net_lookup_host( d->host, &sa ) )
	{
		loop_end( "Unable to look up network target." );
		free( t );
		return NULL;
	}

	// and we already have a port
	sa.sin_port = htons( d->port );

	// make a socket
	d->sock = net_make_sock( 0, 0, &sa );

	// how long do we count down after 
	d->reconn_ct = ctl->net->reconn / ctl->net->io_usec;
	if( ctl->net->reconn % ctl->net->io_usec )
		d->reconn_ct++;

	// init it's mutex
	pthread_mutex_init( &(d->lock), NULL );

	loop_mark_start( "io" );

	// now loop around sending
	while( ctl->run_flags & RUN_LOOP )
	{
		usleep( ctl->net->io_usec );

		// call the right io fn - stdout is different from network
		fires += (*(d->iofp))( d );

		debug_io( "Target %s:%hu bufs %d", d->host, d->port, d->iolist->bufs );
	}

	loop_mark_done( "io", 0, fires );

	// and tear down the mutex
	pthread_mutex_destroy( &(d->lock) );

	// shut down that socket.  Never mind freeing stuff,
	// we are exiting.
	if( d->sock->sock > -1 )
	{
		shutdown( d->sock->sock, SHUT_RDWR );
		close( d->sock->sock );
		d->sock->sock = -1;
	}

	free( t );
	return NULL;
}





