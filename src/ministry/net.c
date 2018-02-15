/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* net.c - networking setup and config                                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"



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
	if( ctl->net->prefix->verbose )
		info( "Connection from %s:%hu gets prefix %s",
			inet_ntoa( h->peer->sin_addr ), ntohs( h->peer->sin_port ),
			h->workbuf );

	return 0;
}


// set the line parser on a host.  We prefer the flat parser, but if
// tokens are on for that type, set the token handler.
int net_set_host_parser( HOST *h, int token_check, int prefix_check )
{
	IPNET *n;

	// this is the default
	h->parser = h->type->flat_parser;

	// are we doing a token check?
	if(   token_check
	 &&   ctl->net->tokens->enable
	 && ( ctl->net->tokens->mask & h->type->token_type ) )
	{
		// token handler is the same for all types
		h->parser = &data_line_token;
		return 0;
	}

	// do we have a prefix for this host?
	if( prefix_check )
	{
		iplist_test_ip( ctl->net->prefix, h->peer->sin_addr.s_addr, &n );
		return net_set_host_prefix( h, n );
	}

	return 0;
}




void net_start_type( NET_TYPE *nt )
{
	throw_fn *fp;
	TCPTH *th;
	int i, j;

	if( !( nt->flags & NTYPE_ENABLED ) )
		return;

	if( nt->flags & NTYPE_TCP_ENABLED )
	{
		// start up our handler threads
		nt->tcp->threads = (TCPTH **) allocz( nt->threads * sizeof( TCPTH * ) );
		for( i = 0; i < nt->threads; i++ )
		{
			th = (TCPTH *) allocz( sizeof( TCPTH ) );

			th->type  = nt;
            th->hosts = (HOST **) allocz( nt->pollmax * sizeof( HOST * ) );
			th->polls = (struct pollfd *) allocz( nt->pollmax * sizeof( struct pollfd ) );
            for( j = 0; j < nt->pollmax; j++ )
            {
                th->polls[j].fd     = -1;  // makes poll ignore this one
                th->polls[j].events = POLL_EVENTS;
            }

			pthread_mutex_init( &(th->lock), NULL );
			nt->tcp->threads[i] = th;

			thread_throw( tcp_watcher, th, i );
		}

		// and start watching the socket
		thread_throw( tcp_loop, nt->tcp, 0 );
	}

	if( nt->flags & NTYPE_UDP_ENABLED )
	{
		if( nt->flags & NTYPE_UDP_CHECKS && ctl->net->prefix )
			fp = &udp_loop_checks;
		else
			fp = &udp_loop_flat;

		for( i = 0; i < nt->udp_count; i++ )
			thread_throw( fp, nt->udp[i], i );
	}

	info( "Started listening for data on %s", nt->label );
}





int net_startup( NET_TYPE *nt )
{
	int i, j;

	if( !( nt->flags & NTYPE_ENABLED ) )
		return 0;

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


    notice( "TCP dead time is %d seconds.", ctl->net->dead_time );

	if( nt->flags & NTYPE_UDP_ENABLED )
		for( i = 0; i < nt->udp_count; i++ )
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

				for( j = 0; j < i; j++ )
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


int net_start( void )
{
	int ret = 0;

	// create our token hash table
	token_init( );

	if( ctl->net->filter_list )
	{
		if( !( ctl->net->filter = iplist_find( ctl->net->filter_list ) ) )
			return -1;

		iplist_explain( ctl->net->filter, NULL, NULL, NULL, NULL );
	}

	if( ctl->net->prefix_list )
	{
		if( !( ctl->net->prefix = iplist_find( ctl->net->prefix_list ) ) )
			return -1;

		iplist_explain( ctl->net->prefix, NULL, NULL, "Prefix", NULL );
	}

	notice( "Starting networking." );

	ret += net_startup( ctl->net->stats );
	ret += net_startup( ctl->net->adder );
	ret += net_startup( ctl->net->gauge );
	ret += net_startup( ctl->net->compat );

	return ret;
}


void net_shutdown( NET_TYPE *nt )
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
		for( i = 0; i < nt->udp_count; i++ )
			if( nt->udp[i]->fd > -1 )
			{
				close( nt->udp[i]->fd );
				nt->udp[i]->fd = -1;
			}


	notice( "Stopped %s", nt->label );
}



void net_stop( void )
{
	notice( "Stopping networking." );

	net_shutdown( ctl->net->stats );
	net_shutdown( ctl->net->adder );
	net_shutdown( ctl->net->gauge );
	net_shutdown( ctl->net->compat );
}


NET_TYPE *net_type_defaults( int type )
{
	const DTYPE *d = data_type_defns + type;
	NET_TYPE *nt;

	nt              = (NET_TYPE *) allocz( sizeof( NET_TYPE ) );
	nt->tcp         = (NET_PORT *) allocz( sizeof( NET_PORT ) );
	nt->tcp->ip     = INADDR_ANY;
	nt->tcp->back   = DEFAULT_NET_BACKLOG;
	nt->tcp->port   = d->port;
	nt->tcp->type   = nt;
	nt->flat_parser = d->lf;
	nt->prfx_parser = d->pf;
	nt->handler     = d->af;
	nt->udp_bind    = INADDR_ANY;
	nt->threads     = d->thrd;
	nt->pollmax     = TCP_MAX_POLLS;
	nt->label       = strdup( d->sock );
	nt->name        = strdup( d->name );
	nt->flags       = NTYPE_ENABLED|NTYPE_TCP_ENABLED|NTYPE_UDP_ENABLED;
	nt->token_type  = d->tokn;

	return nt;
}



NET_CTL *net_config_defaults( void )
{
	NET_CTL *net;

	net              = (NET_CTL *) allocz( sizeof( NET_CTL ) );
	net->dead_time   = NET_DEAD_CONN_TIMER;
	net->rcv_tmout   = NET_RCV_TMOUT;

	net->stats       = net_type_defaults( DATA_TYPE_STATS );
	net->adder       = net_type_defaults( DATA_TYPE_ADDER );
	net->gauge       = net_type_defaults( DATA_TYPE_GAUGE );
	net->compat      = net_type_defaults( DATA_TYPE_COMPAT );

	// create our tokens structure
	net->tokens      = token_setup( );

	return net;
}


#define ntflag( f )			if( config_bool( av ) ) nt->flags |= NTYPE_##f; else nt->flags &= ~NTYPE_##f







int net_config_line( AVP *av )
{
	NET_CTL *n = ctl->net;
	struct in_addr ina;
	char *d, *p, *cp;
	NET_TYPE *nt;
	int i, tcp;
	int64_t v;
	WORDS *w;


	if( !( d = strchr( av->att, '.' ) ) )
	{
		if( attIs( "timeout" ) )
		{
			av_int( v );
			n->dead_time = (time_t) v;
			debug( "Dead connection timeout set to %d sec.", n->dead_time );
		}
		else if( attIs( "rcvTmout" ) )
		{
			av_int( v );
			n->rcv_tmout = (unsigned int) v;
			debug( "Receive timeout set to %u sec.", n->rcv_tmout );
		}
		else if( attIs( "prefixList" ) )
		{
			n->prefix_list = str_copy( av->val, av->vlen );
		}
		else if( attIs( "filterList" ) )
		{
			n->filter_list = str_copy( av->val, av->vlen );
		}
		else
			return -1;

		return 0;
	}

	/* then it's data., statsd. or adder. (or ipcheck) */
	p = d + 1;

	// the names changed, so support both
	if( !strncasecmp( av->att, "stats.", 6 ) || !strncasecmp( av->att, "data.", 5 ) )
		nt = n->stats;
	else if( !strncasecmp( av->att, "adder.", 6 ) )
		nt = n->adder;
	else if( !strncasecmp( av->att, "gauge.", 6 ) || !strncasecmp( av->att, "guage.", 6 ) )
		nt = n->gauge;
	else if( !strncasecmp( av->att, "compat.", 7 ) || !strncasecmp( av->att, "statsd.", 7 ) )
		nt = n->compat;
	else if( !strncasecmp( av->att, "tokens.", 7 ) )
	{
		if( !strcasecmp( p, "enable" ) )
			n->tokens->enable = config_bool( av );
		else if( !strcasecmp( p, "msec" ) || !strcasecmp( p, "lifetime" ) )
		{
			i = atoi( av->val );
			if( i < 10 )
			{
				i = DEFAULT_TOKEN_LIFETIME;
				warn( "Minimum token lifetime is 10msec - setting to %d", i );
			}
			n->tokens->lifetime = i;
		}
		else if( !strcasecmp( p, "hashsize" ) )
		{
			if( !( n->tokens->hsize = hash_size( av->val ) ) )
				return -1;
		}
		else if( !strcasecmp( p, "mask" ) )
		{
			av_int( n->tokens->mask );
		}
		else
			return -1;

		return 0;
	}
	else
		return -1;

	// only a few single-words per connection type
	if( !( d = strchr( p, '.' ) ) )
	{
		if( !strcasecmp( p, "enable" ) )
		{
			ntflag( ENABLED );
			debug( "Networking %s for %s", ( nt->flags & NTYPE_ENABLED ) ? "enabled" : "disabled", nt->label );
		}
		else if( !strcasecmp( p, "label" ) )
		{
			free( nt->label );
			nt->label = strdup( av->val );
		}
		else
			return -1;

		return 0;
	}

	d++;

	// which port struct are we working with?
	if( !strncasecmp( p, "udp.", 4 ) )
		tcp = 0;
	else if( !strncasecmp( p, "tcp.", 4 ) )
		tcp = 1;
	else
		return -1;

	if( !strcasecmp( d, "enable" ) )
	{
		if( tcp )
		{
			ntflag( TCP_ENABLED );
		}
		else
		{
			ntflag( UDP_ENABLED );
		}
	}
	else if( !strcasecmp( d, "threads" ) )
	{
		if( !tcp )
			warn( "Threads is only for TCP connections - there is one thread per UDP port." );
		else
		{
			if( parse_number( av->val, &v, NULL ) == NUM_INVALID )
			{
				err( "Invalid TCP thread count: %s", av->val );
				return -1;
			}
			if( v < 1 || v > 1024 )
			{
				err( "TCP thread count must be 1 <= X <= 1024." );
				return -1;
			}

			nt->threads = v;
		}
	}
	else if( !strcasecmp( d, "pollMax" ) )
	{
		if( !tcp )
			warn( "Pollmax is only for TCP connections." );
		else
		{
			if( parse_number( av->val, &v, NULL ) == NUM_INVALID )
			{
				err( "Invalid TCP pollmax count: %s", av->val );
				return -1;
			}
			if( v < 1 || v > 1024 )
			{
				err( "TCP pollmax must be 1 <= X <= 1024." );
				return -1;
			}

			nt->pollmax = v;
		}
	}
	else if( !strcasecmp( d, "checks" ) )
	{
		if( tcp )
			warn( "To disable prefix checks on TCP, set enable 0 on prefixes." );
		else
		{
			ntflag( UDP_CHECKS );
		}
	}
	else if( !strcasecmp( d, "bind" ) )
	{
		inet_aton( av->val, &ina );
		if( tcp )
			nt->tcp->ip = ina.s_addr;
		else
			nt->udp_bind = ina.s_addr;
	}
	else if( !strcasecmp( d, "backlog" ) )
	{
		if( tcp )
		{
			av_int( v );
			nt->tcp->back = (unsigned short) v;
		}
		else
			warn( "Cannot set a backlog for a udp connection." );
	}
	else if( !strcasecmp( d, "port" ) || !strcasecmp( d, "ports" ) )
	{
		if( tcp )
		{
			av_int( v );
			nt->tcp->port = (unsigned short) v;
		}
		else
		{
			// work out how many ports we have
			w  = (WORDS *) allocz( sizeof( WORDS ) );
			cp = strdup( av->val );
			strwords( w, cp, 0, ',' );
			if( w->wc > 0 )
			{
				nt->udp_count = w->wc;
				nt->udp       = (NET_PORT **) allocz( w->wc * sizeof( NET_PORT * ) );

				debug( "Discovered %d udp ports for %s", w->wc, nt->label );

				for( i = 0; i < w->wc; i++ )
				{
					nt->udp[i]       = (NET_PORT *) allocz( sizeof( NET_PORT ) );
					nt->udp[i]->port = (unsigned short) strtoul( w->wd[i], NULL, 10 );
					nt->udp[i]->type = nt;
				}
			}

			free( cp );
			free( w );
		}
	}
	else
		return -1;

	return 0;
}

#undef ntflag

