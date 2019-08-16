/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* net.c - handles network functions                                       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



int net_lookup_host( char *host, struct sockaddr_in *res )
{
	struct addrinfo *ap, *ai = NULL;

	if( getaddrinfo( host, NULL, NULL, &ai ) )
	{
		err( "Could not look up host %s -- %s", host, Err );
		return -1;
	}

	// get anything?
	if( !ai )
	{
		err( "Found nothing when looking up host %s", host );
		return -1;
	}

	// find an AF_INET answer - we don't do IPv6 yet
	for( ap = ai; ap; ap = ap->ai_next )
		if( ap->ai_family == AF_INET )
			break;

	// none?
	if( !ai )
	{
		err( "Could not find an IPv4 answer for host %s", host );
		freeaddrinfo( ai );
		return -1;
	}

	// grab the first result
	res->sin_family = ap->ai_family;
	res->sin_addr   = ((struct sockaddr_in *) ap->ai_addr)->sin_addr;

	freeaddrinfo( ai );
	return 0;
}


// yes or no for a filter list
int net_ip_check( IPLIST *l, struct sockaddr_in *sin )
{
	IPNET *n;
	int v;

	if( !l || !l->enable )
		return 0;

	v = iplist_test_ip( l, sin->sin_addr.s_addr, &n );

	if( !n )
		v = l->def;

	if( v == IPLIST_NEGATIVE )
		return 1;

	return 0;
}


// set prefix data on a host
int net_set_host_prefix( HOST *h, IPNET *n )
{
	// not a botch - simplifies other callers
	if( !n || !n->text )
		return 0;

	h->ipn = n;

	// change the parser function to one that does prefixing
	h->parser = h->type->prfx_parser;

	// and copy the prefix into the workbuf
	if( !h->workbuf && !( h->workbuf = (char *) allocz( HPRFX_BUFSZ ) ) )
	{
		mem_free_host( &h );
		fatal( "Could not allocate host work buffer" );
		return -1;
	}

	// and make a copy of the prefix for this host
	memcpy( h->workbuf, n->text, n->tlen );
	h->workbuf[n->tlen] = '\0';
	h->plen = n->tlen;

	// set the max line we like and the target to copy to
	h->lmax = HPRFX_BUFSZ - h->plen - 1;
	h->ltarget = h->workbuf + h->plen;

	// report on that?
	if( n->list->verbose )
		info( "Connection from %s:%hu gets prefix %s",
			inet_ntoa( h->peer->sin_addr ), ntohs( h->peer->sin_port ),
			h->workbuf );

	return 0;
}




__attribute__((hot)) void net_token_parser( HOST *h, char *line, int len )
{
	int64_t tval;
	TOKEN *t;

	// have we flagged them already?
	if( h->net->flags & IO_CLOSE )
	{
		debug( "Host already flagged as closed - ignoring line: %s", line );
		return;
	}

	// read the token
	tval = strtoll( line, NULL, 10 );

	// look up a token based on that information
	if( !( t = token_find( h->ip, h->type->token_bit, tval ) ) )
	{
		debug( "Found no token: %p", t );

		// we were expecting a token, so, no.
		h->net->flags |= IO_CLOSE;
		return;
	}

	// burn that token
	token_burn( t );

	// reset the handler function
	net_set_host_parser( h, 0, 1 );
}





// set the line parser on a host.  We prefer the flat parser, but if
// tokens are on for that type, set the token handler.
int net_set_host_parser( HOST *h, int token_check, int prefix_check )
{
	TOKENS *ts = _net->tokens;
	int set_token = 0;
	NET_PFX *p;
	IPNET *n;

	// are we doing a token check?
	if(   token_check
	 &&   ts->enable
	 && ( ts->mask & ( 1 << h->type->token_bit ) ) )
	{
		if( ts->filter )
		{
			iplist_test_ip( ts->filter, h->ip, &n );
			if( n || ts->filter->def == IPLIST_POSITIVE )
				set_token = 1;
		}
		else
			set_token = 1;

		if( set_token )
		{
			// token handler is the same for all types
			h->parser = &net_token_parser;
			return 0;
		}
	}

	// do we have a prefix for this host?
	if( prefix_check )
		for( p = _net->prefix; p; p = p->next )
		{
			if( iplist_test_ip( p->list, h->ip, &n ) != IPLIST_NOMATCH )
				return net_set_host_prefix( h, n );
		}

	return 0;
}




void net_begin_type( NET_TYPE *nt )
{
	throw_fn *fp;
	int i;

	if( !( nt->flags & NTYPE_ENABLED ) )
	{
		info( "Network type %s is not enabled.", nt->name );
		return;
	}

	if( nt->flags & NTYPE_TCP_ENABLED )
	{
		// call the style handler
		(*(nt->tcp_setup))( nt );

		// and start watching the socket
		thread_throw_named_f( tcp_loop, nt->tcp, 0, "tcp_loop_%hu", nt->tcp->port );
	}

	if( nt->flags & NTYPE_UDP_ENABLED )
	{
		if( nt->flags & NTYPE_UDP_CHECKS && _net->prefix )
			fp = &udp_loop_checks;
		else
			fp = &udp_loop_flat;

		for( i = 0; i < nt->udp_count; ++i )
			thread_throw_named_f( fp, nt->udp[i], i, "udp_loop_%hu", nt->udp[i]->port );
	}

	info( "Started listening for data on %s", nt->label );
}





int ntype_startup( NET_TYPE *nt )
{
	int i, j;

	if( !( nt->flags & NTYPE_ENABLED ) )
		return 0;

	// grab our tcp setup/handler fns
	nt->tcp_setup = tcp_styles[nt->tcp_style].setup;
	nt->tcp_hdlr  = tcp_styles[nt->tcp_style].hdlr;

	if( nt->flags & NTYPE_TCP_ENABLED )
	{
		// listen on the port
		nt->tcp->fd = tcp_listen( nt->tcp->port, nt->tcp->ip, nt->tcp->back );
		if( nt->tcp->fd < 0 )
			return -1;

		// init the lock for keeping track of current connections
		pthread_mutex_init( &(nt->lock), NULL );

		info( "Type %s can handle at most %ld connections.", nt->name, ( nt->threads * nt->pollmax ) );
	}

	notice( "Type %s has TCP dead time %ds, TCP handler style %s.",
		nt->name, _net->dead_time, tcp_styles[nt->tcp_style].name );

	if( nt->flags & NTYPE_UDP_ENABLED )
		for( i = 0; i < nt->udp_count; ++i )
		{
			// grab the udp ip variable
			nt->udp[i]->ip   = nt->udp_bind;
			nt->udp[i]->fd = udp_listen( nt->udp[i]->port, nt->udp[i]->ip );
			if( nt->udp[i]->fd < 0 )
			{
				if( nt->flags & NTYPE_TCP_ENABLED )
				{
					close( nt->tcp->fd );
					nt->tcp->fd = -1;
				}

				for( j = 0; j < i; ++j )
				{
					close( nt->udp[j]->fd );
					nt->udp[j]->fd = -1;
				}
				return -2;
			}
			debug( "Bound udp port %hu with socket %d",
					nt->udp[i]->port, nt->udp[i]->fd );
		}

	notice( "Started up %s", nt->label );
	return 0;
}


void ntype_shutdown( NET_TYPE *nt )
{
	int i;

	if( !( nt->flags & NTYPE_ENABLED ) )
		return;

	if( ( nt->flags & NTYPE_TCP_ENABLED ) && nt->tcp->fd > -1 )
	{
		close( nt->tcp->fd );
		nt->tcp->fd = -1;
		pthread_mutex_destroy( &(nt->lock) );
	}

	if( nt->flags & NTYPE_UDP_ENABLED )
		for( i = 0; i < nt->udp_count; ++i )
			if( nt->udp[i]->fd > -1 )
			{
				close( nt->udp[i]->fd );
				nt->udp[i]->fd = -1;
			}


	notice( "Stopped %s", nt->label );
}


void net_begin( void )
{
	NET_TYPE *nt;

	for( nt = _net->ntypes; nt; nt = nt->next )
		net_begin_type( nt );
}


int net_start( void )
{
	NET_TYPE *t;
	int ret = 0;
	NET_PFX *p;

	// create our token hash table
	if( token_init( ) )
		return -1;

	if( _net->filter_list )
	{
		if( !( _net->filter = iplist_find( _net->filter_list ) ) )
			return -1;

		iplist_explain( _net->filter, NULL, NULL, NULL, NULL );
	}

	if( _net->prefix )
	{
		_net->prefix = (NET_PFX *) mem_reverse_list( _net->prefix );

		for( p = _net->prefix; p; p = p->next )
		{
			if( !( p->list = iplist_find( p->name ) ) )
				return -1;

			iplist_set_text( p->list, p->text, p->tlen );
			iplist_explain( p->list, NULL, NULL, "Prefix", NULL );
		}
	}

	notice( "Starting networking." );

	// convert dead time to nsec
	_net->dead_nsec = BILLION * _net->dead_time;

	for( t = _net->ntypes; t; t = t->next )
		ret += ntype_startup( t );

	return ret;
}




void net_stop( void )
{
	NET_TYPE *t;

	notice( "Stopping networking." );

	for( t = _net->ntypes; t; t = t->next )
		ntype_shutdown( t );

	token_finish( );
}



