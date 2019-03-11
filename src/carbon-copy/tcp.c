/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* tcp.c - handles TCP connections                                         *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "carbon_copy.h"


static inline void tcp_hbuf_post( HBUFS *hb, int idx, int replace )
{
	io_buf_post( hb->rule->targets[idx], hb->bufs[idx] );
	hb->bufs[idx] = NULL;

	if( replace )
		hb->bufs[idx] = mem_new_iobuf( IO_BUF_SZ );
}


static inline void tcp_flush_host( HOST *h, int replace )
{
	HBUFS *hb;
	int i;

	// send all bufs that have data in
	for( hb = h->hbufs; hb; hb = hb->next )
		switch( hb->rule->type )
		{
			case RTYPE_REGEX:
				// single buffer, multi-target
				if( hb->bufs[0] && hb->bufs[0]->len > 0 )
					tcp_hbuf_post( hb, 0, replace );
				break;

			case RTYPE_HASH:
				// multiple buffers
				for( i = 0; i < hb->bcount; i++ )
					if( hb->bufs[i] && hb->bufs[i]->len > 0 )
						tcp_hbuf_post( hb, i, replace );
				break;
		}
}





void tcp_close_host( HOST *h )
{
	io_disconnect( h->net );
	debug( "Closed connection from host %s.", h->net->name );

	// flush buffers
	tcp_flush_host( h, 0 );

	// give us a moment
	usleep( 10000 );
	mem_free_host( &h );
}



HOST *tcp_get_host( int sock, NET_PORT *np )
{
	struct sockaddr_in from;
	socklen_t sz;
	IPNET *ipn;
	HOST *h;
	int d;

	sz = sizeof( from );

	if( ( d = accept( sock, (struct sockaddr *) &from, &sz ) ) < 0 )
	{
		// broken
		err( "Accept error -- %s", Err );
		np->errors.count++;
		return NULL;
	}

	// are we doing blacklisting/whitelisting?
	if( net_ip_check( ctl->net->filter, &from ) != 0 )
	{
		if( ctl->net->filter->verbose )
			notice( "Denying connection from %s:%hu based on ip check.",
				inet_ntoa( from.sin_addr ), ntohs( from.sin_port ) );

		// keep track
		np->drops.count++;

		// and done
		shutdown( d, SHUT_RDWR );
		close( d );
		return NULL;
	}

	if( !( h = mem_new_host( &from, RR_NETBUF_SZ ) ) )
		fatal( "Could not allocate new host." );

	h->net->fd = d;
	h->type    = np->type;

	// assume type-based handler functions
	h->parser    = np->type->flat_parser;

	// maybe set a prefix
	iplist_test_ip( ctl->net->prefix, from.sin_addr.s_addr, &ipn );
	if( net_set_host_prefix( h, ipn ) )
	{
		np->errors.count++;
		mem_free_host( &h );
		return NULL;
	}

	// and try to get a buffer set
	h->hbufs = relay_buf_set( );

	np->accepts.count++;
	return h;
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

__attribute__((hot)) void tcp_connection( THRD *t )
{
	int rv, quiet, quietmax;
	int64_t ts, last;
	struct pollfd p;
	SOCK *n;
	HOST *h;

	h = (HOST *) t->arg;
	n = h->net;

	info( "Accepted %s connection from host %s.", h->type->label, n->name );

	// mark having a connection
	lock_ntype( h->type );
	h->type->conns++;
	unlock_ntype( h->type );

	// set the overlength warn threshold if we are prefixing
	h->olwarn = 32;

	p.fd     = n->fd;
	p.events = POLL_EVENTS;
	quiet    = 0;
	quietmax = ctl->net->dead_time;
	last     = tsll( ctl->proc->curr_time );

	while( RUNNING( ) )
	{
		// get the time
		ts = tsll( ctl->proc->curr_time );

		// do we flush lines from this host?
		if( ( ts - last ) >= ctl->net->flush_nsec )
		{
			tcp_flush_host( h, 1 );
			last = ts;
		}

		if( ( rv = poll( &p, 1, 1000 ) ) < 0 )
		{
			// don't sweat interruptions
			if( errno == EINTR )
				continue;

			warn( "Poll error talk to host %s -- %s",
				n->name, Err );
			break;
		}

		if( !rv )
		{
			// timeout?
			if( ++quiet > quietmax )
			{
				notice( "Connection from host %s timed out.",
					n->name );
				break;
			}
			continue;
		}

		// they went away?
		if( p.revents & POLLHUP )
		{
			debug( "Received pollhup event from %s", n->name );
			break;
		}

		// mark us as having data
		quiet = 0;

		// and start reading
		n->flags |= IO_CLOSE_EMPTY;

		// we need to loop until there's nothing left to read
		while( io_read_data( n ) > 0 )
		{
			// do we have anything
			if( !n->in->len )
			{
				debug( "No incoming data from %s", n->name );
				break;
			}

			// remove the close-empty flag
			// we only want to close if our first
			// read finds nothing
			n->flags &= ~IO_CLOSE_EMPTY;

			// and parse that buffer
			n->in->len = relay_parse_buf( h, n->in );
		}

		// did we get something?  or are we done?
		if( n->flags & IO_CLOSE )
		{
			debug( "Host %s flagged as closed.", n->name );
			break;
		}
	}

	info( "Closing connection to host %s after relaying %lu : %lu lines.",
			n->name, h->relayed, h->lines );

	// mark closing a connection
	lock_ntype( h->type );
	h->type->conns--;
	unlock_ntype( h->type );

	tcp_close_host( h );
}



void tcp_loop( THRD *t )
{
	struct pollfd p;
	NET_PORT *n;
	HOST *h;
	int rv;

	n = (NET_PORT *) t->arg;

	p.fd     = n->fd;
	p.events = POLL_EVENTS;

	loop_mark_start( "tcp" );

	while( RUNNING( ) )
	{
		if( ( rv = poll( &p, 1, 500 ) ) < 0 )
		{
			if( errno != EINTR )
			{
				err( "Poll error on %s -- %s", n->type->label, Err );
				loop_end( "polling error on tcp socket" );
				break;
			}
		}
		else if( !rv )
			continue;

		// go get that then
		if( p.revents & POLL_EVENTS )
		{
			if( ( h = tcp_get_host( p.fd, n ) ) )
			{
				thread_throw_named_f( tcp_connection, h, 0, "h_%08x:%04x",
					ntohl( h->net->peer.sin_addr.s_addr ),
					ntohs( h->net->peer.sin_port ) );
			}
		}
	}

	loop_mark_done( "tcp", 0, 0 );
}


int tcp_listen( unsigned short port, uint32_t ip, int backlog )
{
	struct sockaddr_in sa;
	int s, so;

	if( ( s = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
	{
		err( "Unable to make tcp listen socket -- %s", Err );
		return -1;
	}

	so = 1;
	if( setsockopt( s, SOL_SOCKET, SO_REUSEADDR, &so, sizeof( int ) ) )
	{
		err( "Set socket options error for listen socket -- %s", Err );
		close( s );
		return -2;
	}

	memset( &sa, 0, sizeof( struct sockaddr_in ) );
	sa.sin_family = AF_INET;
	sa.sin_port   = htons( port );

	// ip as well?
	sa.sin_addr.s_addr = ( ip ) ? ip : INADDR_ANY;

	// try to bind
	if( bind( s, (struct sockaddr *) &sa, sizeof( struct sockaddr_in ) ) < 0 )
	{
		err( "Bind to %s:%hu failed -- %s",
			inet_ntoa( sa.sin_addr ), port, Err );
		close( s );
		return -3;
	}

	if( !backlog )
		backlog = 5;

	if( listen( s, backlog ) < 0 )
	{
		err( "Listen error -- %s", Err );
		close( s );
		return -4;
	}

	info( "Listening on tcp port %s:%hu for connections.", inet_ntoa( sa.sin_addr ), port );

	return s;
}



