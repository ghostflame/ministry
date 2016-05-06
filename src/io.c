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


int io_read_data( NSOCK *s )
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
#ifdef DEBUG_IO
			debug( "Poll timed out try to write to %s", s->name );
#endif
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
#ifdef DEBUG_IO
		debug( "Socket is -1, so not connected." );
#endif
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
		err( "Unable to connect to %s:%hu -- %s",
			inet_ntoa( s->peer.sin_addr ), ntohs( s->peer.sin_port ),
			Err );
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




static inline void io_decr_buf( IOBUF *buf )
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



// attach a buffer for sending
// this is called from multiple threads
// they should never fight over a buffer
// but WILL fight over targets
//
// After a call to this function, the buffer
// is 'owned' by the targets and should not be
// written to.  It will get recycled after
// sending
void io_buf_send( IOBUF *buf )
{
	TARGET *t;
	IOLIST *i;

	if( !buf )
		return;

	// it does contain something, right?
	if( !buf->len )
	{
		debug( "Empty buffer passed to io_buf_send." );
		mem_free_buf( &buf );
		return;
	}

	// set the buffer to be handled by each target
	// we have to do it here to prevent us getting
	// tripped up midway through handing it to the
	// io threads
	buf->refs = ctl->net->tcount;

	// as soon as the buffer is assigned to one target
	// it's ref count is subject to multithreaded activity

	// give a ref to each io thread
	for( t = ctl->net->targets; t; t = t->next )
	{
		// unless it's full already in which case
		// decrement and maybe free
		if( t->bufs > ctl->net->max_bufs )
		{
#ifdef DEBUG_IO
			debug( "Dropping buffer to target %s:%hu", t->host, t->port );
#endif
			io_decr_buf( buf );
			continue;
		}
		

		// create the iolist structure
		// and hand it off into the queue
		i = mem_new_iolist( );
		i->buf = buf;

		// and attach it
		lock_target( t );

		if( !t->qend )
			t->qend = t->qhead = i;
		else
		{
			// make the doubly-linked list
			i->next        = t->qhead;
			t->qhead->prev = i;
			t->qhead       = i;
		}

		t->bufs++;

#ifdef DEBUG_IO
		debug( "Target %s:%hu buf count is now %d",
			t->host, t->port, t->bufs );
#endif

		unlock_target( t );
	}
}


void io_grab_buffer( TARGET *t )
{
	IOLIST *l;

	lock_target( t );

	t->curr_off  = 0;
	t->curr_len  = 0;
	t->sock->out = NULL;

	if( !t->qend )
	{
		unlock_target( t );
		return;
	}

	// take one off the end
	l = t->qend;
	t->qend = l->prev;
	t->bufs--;

	// was that the last one?
	if( t->qend )
		t->qend->next = NULL;
	else
		t->qhead = NULL;

	unlock_target( t );

	t->sock->out = l->buf;
	t->curr_len  = l->buf->len;

	mem_free_iolist( &l );

#ifdef DEBUG_IO
	debug( "Target %s:%hu has buffer %p (%d)",
		t->host, t->port, t->sock->out, t->curr_len );
#endif
}


void io_send_loop( TARGET *t )
{
	NSOCK *s;

	s = t->sock;

	while( ctl->run_flags & RUN_LOOP )
	{
		// at the top to allow continue to invoke the sleep
		usleep( ctl->net->io_usec );

#ifdef DEBUG_IO
		debug( "Target %s:%hu loop.", t->host, t->port );
#endif

		// are we waiting to reconnect?
		if( t->countdown > 0 )
		{
			t->countdown--;
#ifdef DEBUG_IO
			debug( "Target %s:%hu countdown is now %d",
				t->host, t->port, t->countdown );
#endif
			continue;
		}

		// keep trying if we are not connected
		if( io_connected( t->sock ) < 0
		 && io_connect( t ) < 0 )
		{
			// don't try to reconnect at once
			// sleep a few seconds
			t->countdown = t->reconn_ct;
			notice( "Resting for %d ticks before reconnecting to %s:%hu",
					t->countdown, t->host, t->port );
			continue;
		}

		// right, were we working on anything?
		if( !s->out )
			io_grab_buffer( t );

		// while there's something to send
		while( s->out )
		{
			// try to send the out buffer
			t->curr_off += io_write_data( s, t->curr_off );

			// did we have problems?
			if( s->flags & HOST_CLOSE )
			{
#ifdef DEBUG_IO
				debug( "Disconnecting from target %s:%hu",
					t->host, t->port );
#endif
				net_disconnect( &(s->sock), "send target" );
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
#ifdef DEBUG_IO
				debug( "Target %s:%hu finishing with %d bytes to go.",
					t->host, t->port, t->curr_len - t->curr_off );
#endif
				// try again later
				break;
			}
		}
	}
}



void *io_loop( void *arg )
{
	struct sockaddr_in sa;
	TARGET *d;
	THRD *t;

	t = (THRD *) arg;
	d = (TARGET *) t->arg;

	// find out where we are connecting to
	if( net_lookup_host( d->host, &sa ) )
	{
		loop_end( "Unable to loop up network target." );
		free( t );
		return NULL;
	}

	// and we already have a port
	sa.sin_port   = htons( d->port );

	// make a socket
	d->sock = net_make_sock( 0, 0, &sa );

	// how long do we count down after 
	d->reconn_ct = ctl->net->reconn / ctl->net->io_usec;
	if( ctl->net->reconn % ctl->net->io_usec )
		d->reconn_ct++;

	// init it's mutex
	pthread_mutex_init( &(d->lock), NULL );


	// now loop around sending
	loop_mark_start( "io" );

	io_send_loop( d );

	loop_mark_done( "io" );


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


void io_start( void )
{
	TARGET *t;

	// make a default target if we have none
	if( ! ctl->net->targets )
	{
		t = (TARGET *) allocz( sizeof( TARGET ) );
		t->host = str_dup( DEFAULT_TARGET_HOST, 0 );
		t->port = DEFAULT_TARGET_PORT;

		ctl->net->targets = t;
		ctl->net->tcount  = 1;

		info( "Created default target of %s:%hu", t->host, t->port );
	}

	// start one loop for each target
	for( t = ctl->net->targets; t; t = t->next )
		thread_throw( io_loop, t );
}


void io_stop( void )
{
	TARGET *t;

	for( t = ctl->net->targets; t; t = t->next )
		pthread_mutex_destroy( &(t->lock) );
}



