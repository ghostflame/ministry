/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* tcp.c - handles TCP connections                                         *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "ministry.h"



void tcp_disconnect( int *sock, char *name )
{
	if( shutdown( *sock, SHUT_RDWR ) )
		err( "Shutdown error on connection with %s -- %s",
			name, Err );

	close( *sock );
	*sock = -1;
}


void tcp_close_host( HOST *h )
{
	tcp_disconnect( &(h->net->sock), h->net->name );
	debug( "Closed connection from host %s.", h->net->name );

	// give us a moment
	usleep( 10000 );
	mem_free_host( &h );
}



HOST *tcp_get_host( int sock, NET_PORT *np )
{
	struct sockaddr_in from;
	socklen_t sz;
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
	if( net_ip_check( &from ) != 0 )
	{
		if( ctl->net->iplist->verbose )
			notice( "Denying connection from %s:%hu based on ip check.",
				inet_ntoa( from.sin_addr ), ntohs( from.sin_port ) );

		// keep track
		np->drops.count++;

		// and done
		shutdown( d, SHUT_RDWR );
		close( d );
		return NULL;
	}

	if( !( h = mem_new_host( &from, MIN_NETBUF_SZ ) ) )
		fatal( "Could not allocate new host." );

	h->net->sock = d;
	h->type      = np->type;

	// assume type-based handler functions
	// and maybe set a profile
	if( net_set_host_parser( h, 1, 1 ) )
	{
		np->errors.count++;
		return NULL;
	}

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

__attribute__((hot)) void *tcp_watcher( void *arg )
{
	int rv, quiet, quietmax;
	struct pollfd p;
	NSOCK *n;
	THRD *t;
	HOST *h;

	t = (THRD *) arg;
	h = (HOST *) t->arg;
	n = h->net;

	info( "Accepted %s connection from host %s.", h->type->label, n->name );

	// mark having a connection
	lock_ntype( h->type );
	h->type->conns++;
	unlock_ntype( h->type );

	p.fd     = n->sock;
	p.events = POLL_EVENTS;
	quiet    = 0;
	quietmax = ctl->net->dead_time;

	while( ctl->run_flags & RUN_LOOP )
	{
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
		n->flags |= HOST_CLOSE_EMPTY;

		// we need to loop until there's nothing left to read
		while( io_read_data( n ) > 0 )
		{
			// data_parse_buf can set this, so let's not carry
			// on reading from a spammy source if we don't like them
			if( n->flags & HOST_CLOSE )
				break;

			// do we have anything
			if( !n->in->len )
			{
				debug( "No incoming data from %s", n->name );
				break;
			}

			// remove the close-empty flag
			// we only want to close if our first
			// read finds nothing
			n->flags &= ~HOST_CLOSE_EMPTY;

			// and parse that buffer
			data_parse_buf( h, n->in );
		}

		// did we get something?  or are we done?
		if( n->flags & HOST_CLOSE )
		{
			debug( "Host %s flagged as closed.", n->name );
			break;
		}
	}

	info( "Closing connection to host %s after %lu data point%s.",
			n->name, h->points, ( h->points == 1 ) ? "" : "s" );

	// mark closing a connection
	lock_ntype( h->type );
	h->type->conns--;
	unlock_ntype( h->type );

	tcp_close_host( h );

	free( t );
	return NULL;
}



void *tcp_loop( void *arg )
{
	struct pollfd p;
	NET_PORT *n;
	THRD *t;
	HOST *h;
	int rv;

	t = (THRD *) arg;
	n = (NET_PORT *) t->arg;

	p.fd     = n->sock;
	p.events = POLL_EVENTS;

	loop_mark_start( "tcp" );

	while( ctl->run_flags & RUN_LOOP )
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
				thread_throw( tcp_watcher, h );
		}
	}

	loop_mark_done( "tcp", 0, 0 );

	free( t );
	return NULL;
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



