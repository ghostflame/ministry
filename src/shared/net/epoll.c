/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* tcp/epoll.c - handles TCP connections with epoll                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"


void tcp_epoll_add_host( TCPTH *th, HOST *h )
{
	if( th->curr >= th->type->pollmax )
	{
		twarn( "Closing socket to host %s because we cannot take on any more.",
			h->net->name );
		tcp_close_host( h );
		return;
	}

	// keep count
	++(th->curr);

	// add that in
	h->ep_evt.events   = EPOLL_EVENTS;
	h->ep_evt.data.ptr = h;
	epoll_ctl( th->ep_fd, EPOLL_CTL_ADD, h->net->fd, &(h->ep_evt) );

	tinfo( "Accepted %s connection from host %s curr %ld.",
		h->type->label, h->net->name, th->curr );
}


__attribute__((hot)) void tcp_epoll_handler( TCPTH *th, struct epoll_event *e, HOST *h )
{
	SOCK *n = h->net;

	if( !( e->events & POLL_EVENTS ) )
	{
		if( (_proc->curr_tval - h->last) > _net->dead_nsec )
		{
			tnotice( "Connection from host %s timed out.", n->name );
			n->flags |= IO_CLOSE;
		}
		return;
	}

	if( e->events & POLLHUP )
	{
		tdebug( "Received pollhup event from %s", n->name );

		// close host
		n->flags |= IO_CLOSE;
		return;
	}

	// host has data
	h->last = _proc->curr_tval;
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
		(*(h->type->buf_parser))( h, n->in );
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

__attribute__((hot)) void tcp_epoll_thread( THRD *td )
{
	struct epoll_event *ep;
	HOST *h, *prv, *nxt;
	int64_t i;
	TCPTH *th;
	int rv;

	th = (TCPTH *) td->arg;

	// grab our thread id and number
	th->tid = td->id;
	th->num = td->num;

	while( RUNNING( ) )
	{
		// take on any any new sockets
		while( th->waiting )
		{
			lock_tcp( th );

			h = th->waiting;
			th->waiting = h->next;

			unlock_tcp( th );

			// link it up
			h->next   = th->hlist;
			th->hlist = h;

			tcp_epoll_add_host( th, h );
		}

		if( !th->curr )
		{
			// sleep a little
			// check again
			microsleep( 50000 );
			continue;
		}

		// and wait for something
		if( ( rv = epoll_wait( th->ep_fd, th->ep_events, th->type->pollmax, 500 ) ) < 0 )
		{
			// don't sweat interruptions
			if( errno == EINTR )
				continue;

			twarn( "Epoll error -- %s", Err );
			break;
		}

		// run through the answers
		for( i = 0, ep = th->ep_events; i < rv; ++i, ++ep )
			tcp_epoll_handler( th, ep, (HOST *) ep->data.ptr );


		// run through the list of hosts, looking for ones to close
		for( prv = NULL, h = th->hlist; h; h = nxt )
		{
			nxt = h->next;

			// is this one OK?
			if( !( h->net->flags & IO_CLOSE ) )
			{
				prv = h;
				continue;
			}

			// step over this host
			if( prv )
				prv->next = nxt;
			else
				th->hlist = nxt;

			tcp_close_active_host( h );
			th->curr--;
		}

		// and sleep a very little, to avoid epoll-spam
		//microsleep( 2000 );
	}

	// close everything!
	for( h = th->hlist; h; h = nxt )
	{
		nxt = h->next;
		tcp_close_active_host( h );
		th->curr--;
	}
}


// uses the same handler as pool
// tcp_epoll_handler == tcp_pool_handler


void tcp_epoll_setup( NET_TYPE *nt )
{
	TCPTH *th;
	int i;

	// create an epoll fd for each thread
	nt->tcp->threads = (TCPTH **) allocz( nt->threads * sizeof( TCPTH * ) );
	for( i = 0; i < nt->threads; ++i )
	{
		th = (TCPTH *) allocz( sizeof( TCPTH ) );

		th->type      = nt;
		th->ep_fd     = epoll_create1( 0 );
		// make space for the return events
		th->ep_events = (struct epoll_event *) allocz( nt->pollmax * sizeof( struct epoll_event ) );

		pthread_mutex_init( &(th->lock), NULL );
		nt->tcp->threads[i] = th;

		thread_throw_named_f( tcp_epoll_thread, th, i, "tcp_epoll_%d", i );
	}
}


