/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* tcp_thrd.c - handles TCP connections in a single thread                 *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "ministry.h"


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

__attribute__((hot)) void *tcp_thrd_handler( void *arg )
{
	struct pollfd pf;
	THRD *td;
	SOCK *n;
	HOST *h;
	int rv;

	td = (THRD *) arg;
	h  = (HOST *) td->arg;
	n  = h->net;

	pf.fd     = h->fd;
	pf.events = POLL_EVENTS;

	while( RUNNING( ) )
	{
		// poll all active connections
		if( ( rv = poll( &pf, 1, 2000 ) ) < 0 )
		{
			// don't sweat interruptions
			if( errno == EINTR )
				continue;

			twarn( "Poll error -- %s", Err );
			break;
		}

		// anything to do?
		if( !pf.revents )
		{
			if( (ctl->proc->curr_time.tv_sec - h->last) > ctl->net->dead_time )
			{
				tnotice( "Connection from host %s timed out.", n->name );
				n->flags |= IO_CLOSE;
				break;
			}

			continue;
		}

		// Hung up?
		if( pf.revents & POLLHUP )
		{
			tdebug( "Received pollhup event from %s", n->name );

			// close host
			n->flags |= IO_CLOSE;
			break;
		}
		
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

		// are we done with this one?
		if( n->flags & IO_CLOSE )
			break;

	}

	// close everything!
	tcp_close_active_host( h );

	free( td );
	return NULL;
}





