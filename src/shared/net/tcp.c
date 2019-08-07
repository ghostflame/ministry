/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* tcp.c - handles TCP connections                                         *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"


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

void tcp_throw_thread( HOST *h )
{
	// name the thread after the type and remote host
	thread_throw_named_f( &tcp_thrd_thread, h, 0, "%c_%08x:%04x",
		*(h->type->name), ntohl( h->net->peer.sin_addr.s_addr ),
		ntohs( h->net->peer.sin_port ) );
}


// details of the different styles
const struct tcp_style_data tcp_styles[TCP_STYLE_MAX] =
{
	{
		.name  = "pool",
		.style = TCP_STYLE_POOL,
		.setup = &tcp_pool_setup,
		.hdlr  = &tcp_choose_thread,
	},
	{
		.name  = "thread",
		.style = TCP_STYLE_THRD,
		.setup = &tcp_thrd_setup,
		.hdlr  = &tcp_throw_thread,
	},
	{
		.name  = "epoll",
		.style = TCP_STYLE_EPOLL,
		.setup = &tcp_epoll_setup,
		.hdlr  = &tcp_choose_thread,
	}
};



void tcp_close_host( HOST *h )
{
	io_disconnect( h->net );
	debug( "Closed connection from host %s.", h->net->name );

	// give us a moment
	usleep( 5000 );

	mem_free_host( &h );
}

void tcp_close_active_host( HOST *h )
{
	double ctime;

	ctime = ( (double) ( get_time64( ) - h->connected ) ) / BILLIONF;

	info( "Closing connection to host %s after %.3fs and %lu data point%s.",
	    h->net->name, ctime,
		h->lines, VAL_PLURAL( h->lines ) );

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
	if( net_ip_check( _net->filter, &from ) != 0 )
	{
		if( _net->filter && _net->filter->verbose )
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
	h->port    = np;
	h->type    = np->type;

	// default handlers/parsers
	h->parser   = h->type->flat_parser;
	h->handler  = h->type->handler;
	h->reeiver  = h->type->buf_parser;;

	// set the last time to now
	h->last = _proc->curr_tval;
	// and the connected time
	h->connected = get_time64( );

	// assume type-based handler functions
	// and maybe set a profile
	if( net_set_host_parser( h, 1, 1 ) )
	{
		np->errors.count++;
		return NULL;
	}

	// and keep score against the type
	lock_ntype( h->type );
	h->type->conns++;
	unlock_ntype( h->type );

	np->accepts.count++;
	return h;
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
				(*(h->type->tcp_hdlr))( h );
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



