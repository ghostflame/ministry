/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* tcp.c - handles TCP connections                                         *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "ministry.h"




void tcp_close_host( HOST *h )
{
	io_disconnect( h->net );
	debug( "Closed connection from host %s.", h->net->name );

	// give us a moment
	usleep( 1000 );

	mem_free_host( &h );
}

void tcp_close_active_host( HOST *h )
{
	double ctime;

	ctime = ( (double) ( get_time64( ) - h->connected ) ) / BILLIONF;

	info( "Closing connection to host %s after %.3fs and %lu data point%s.",
	    h->net->name, ctime,
		h->points, ( h->points == 1 ) ? "" : "s" );

	// mark closing a connection
	lock_ntype( h->type );
	h->type->conns--;
	unlock_ntype( h->type );

	tcp_close_host( h );
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
	if( net_ip_check( ctl->net->filter, &from ) != 0 )
	{
		if( ctl->net->filter && ctl->net->filter->verbose )
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

	h->net->fd = d;
	h->type    = np->type;

	// set the last time to now
	h->last = ctl->proc->curr_time.tv_sec;
	// and the connected time
	h->connected = get_time64( );

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


#define terr( fmt, ... )        err( "[TCP:%03d] " fmt, th->num, ##__VA_ARGS__ )
#define twarn( fmt, ... )       warn( "[TCP:%03d] " fmt, th->num, ##__VA_ARGS__ )
#define tnotice( fmt, ... )     notice( "[TCP:%03d] " fmt, th->num, ##__VA_ARGS__ )
#define tinfo( fmt, ... )       info( "[TCP:%03d] " fmt, th->num, ##__VA_ARGS__ )
#define tdebug( fmt, ... )      debug( "[TCP:%03d] " fmt, th->num, ##__VA_ARGS__ )



__attribute__((hot)) void tcp_find_slot( TCPTH *th, HOST *h )
{
	int64_t i;

	// are we full?
	if( th->curr == th->type->pollmax || th->pmin < 0 )
	{
		twarn( "Closing socket to host %s because we cannot take on any more.",
		       h->net->name );
		tcp_close_host( h );
		return;
	}

	tinfo( "Accepted %s connection from host %s into slot %ld.", h->type->label, h->net->name, th->pmin );

	// find the first free slot
	i = th->pmin;

	// is this the new highest?
	if( i >= th->pmax )
		th->pmax = i + 1;

	// set this host up in the poll structure
	th->polls[i].fd = h->net->fd;
	th->hosts[i]    = h;

	// count it
	th->curr++;

	// and keep score against the type
	lock_ntype( h->type );
	h->type->conns++;
	unlock_ntype( h->type );

	// find the next free slot
	for( i += 1; i < th->type->pollmax; i++ )
		if( th->polls[i].fd < 0 )
		{
			th->pmin = i;
			break;
		}

	// this will bark if we ever screw up
	if( i == th->type->pollmax )
	{
		th->pmin = -1;
		twarn( "Setting pmin to -1." );
	}
}



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



void tcp_choose_thread( NET_PORT *n, HOST *h )
{
	uint64_t v, w;
	TCPTH *t;

	// choose thread based on host and port
	v = h->peer->sin_port;
	v = ( v << 32 ) + h->ip;

	// use an algorithm more likely to give a better spread
	// first modulo a large prime, then the number of threads
	w = v % TCP_MODULO_PRIME;
	t = n->threads[w % h->type->threads];

	// put it in the waiting queue
	lock_tcp( t );

	h->next = t->waiting;
	t->waiting = h;

	unlock_tcp( t );
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
				tcp_choose_thread( n, h );
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



