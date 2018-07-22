/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* tcp.c - handles TCP connections                                         *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "ministry.h"





#define terr( fmt, ... )        err( "[TCP:%03d] " fmt, th->num, ##__VA_ARGS__ )
#define twarn( fmt, ... )       warn( "[TCP:%03d] " fmt, th->num, ##__VA_ARGS__ )
#define tnotice( fmt, ... )     notice( "[TCP:%03d] " fmt, th->num, ##__VA_ARGS__ )
#define tinfo( fmt, ... )       info( "[TCP:%03d] " fmt, th->num, ##__VA_ARGS__ )
#define tdebug( fmt, ... )      debug( "[TCP:%03d] " fmt, th->num, ##__VA_ARGS__ )



__attribute__((hot)) void tcp_handler( TCPTH *th, struct pollfd *p, HOST *h )
{
	SOCK *n = h->net;

	if( !( p->revents & POLL_EVENTS ) )
	{
		if( (ctl->proc->curr_time.tv_sec - h->last) > ctl->net->dead_time )
		{
			tnotice( "Connection from host %s timed out.", n->name );
			n->flags |= IO_CLOSE;
		}
		return;
	}

	if( p->revents & POLLHUP )
	{
		tdebug( "Received pollhup event from %s", n->name );

		// close host
		n->flags |= IO_CLOSE;
		return;
	}

	// host has data
	h->last = ctl->proc->curr_time.tv_sec;
	n->flags |= IO_CLOSE_EMPTY;

	// we need to loop until there's nothing left to read
	while( io_read_data( n ) > 0 )
	{
		// data_parse_buf can set this, so let's not carry
		// on reading from a spammy source if we don't like them
		if( n->flags & IO_CLOSE )
			break;

		// do we have anything
		if( !n->in->len )
		{
			tdebug( "No incoming data from %s", n->name );
			break;
		}

		// remove the close-empty flag
		// we only want to close if our first
		// read finds nothing
		n->flags &= ~IO_CLOSE_EMPTY;

		// and parse that buffer
		data_parse_buf( h, n->in );
	}
}



//  This function is an annoying mix of io code and processing code,
//  but it is like that to address data splitting across multiple sends,
//  and not on line breaks
//
//  So we have to keep looping around read, looking for more data until we
//  don't get any.  Then we need to loop around processing until there's no
//  data left.  We need to only barf on no data the first time around, thus
//  the empty flag.  We need to keep any partial lines for the next read call
//  to push back to the start of the buffer.
//

__attribute__((hot)) void *tcp_watcher( void *arg )
{
	struct pollfd *pf;
	int64_t i, max;
	TCPTH *th;
	THRD *td;
	HOST *h;
	int rv;

	td = (THRD *) arg;
	th = (TCPTH *) td->arg;

	// grab our thread id and number
	th->tid = td->id;
	th->num = td->num;

	while( RUNNING( ) )
	{
		// take on any any new sockets
		while( th->waiting )
		{
			h = th->waiting;
			th->waiting = h->next;

			tcp_find_slot( th, h );
		}

		if( !th->curr )
		{
			// sleep a little
			// check again
			usleep( 50000 );
			continue;
		}

		// poll all active connections
		if( ( rv = poll( th->polls, th->pmax, 500 ) ) < 0 )
		{
			// don't sweat interruptions
			if( errno == EINTR )
				continue;

			twarn( "Poll error -- %s", Err );
			break;
		}

		// run through the answers
		for( i = 0, pf = th->polls; i < th->pmax; i++, pf++ )
			if( pf->fd >= 0 )
				tcp_handler( th, pf, th->hosts[i] );

		// and run through looking for connections to close
		max = 0;
		for( i = 0; i < th->pmax; i++ )
		{
			// look for valid hosts structures
			if( !( h = th->hosts[i] ) )
			    continue;

			// are we done?  If not, track max
			if( !( h->net->flags & IO_CLOSE ) )
			{
				max = i;
				continue;
			}

			tcp_close_active_host( h );

			// and update our structs
			th->hosts[i] = NULL;
			th->polls[i].fd = -1;

			// and counters
			th->curr--;

			// is this a new first-available-slot?
			if( i < th->pmin )
			    th->pmin = i;
		}

		// have we reduced the max?
		if( ( max + 1 ) < th->pmax )
			th->pmax = max + 1;

		// and sleep a very little, to avoid poll-spam
		//usleep( 1000 );
	}

	// close everything!
	for( i = 0; i < th->pmax; i++ )
	{
		if( ( h = th->hosts[i] ) )
			tcp_close_active_host( h );
	}

	free( td );
	return NULL;
}



void tcp_choose_thread( HOST *h )
{
	uint64_t v, w;
	TCPTH *t;

	// choose thread based on host and port
	v = h->peer->sin_port;
	v = ( v << 32 ) + h->ip;

	// use an algorithm more likely to give a better spread
	// first modulo a large prime, then the number of threads
	w = v % TCP_MODULO_PRIME;
	t = h->port->threads[w % h->type->threads];

	// put it in the waiting queue
	lock_tcp( t );

	h->next = t->waiting;
	t->waiting = h;

	unlock_tcp( t );
}



