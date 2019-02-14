/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* net.c - networking setup and config                                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "carbon_copy.h"





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






void net_start_type( NET_TYPE *nt )
{
	throw_fn *fp;
	char buf[16];
	int i;

	if( !( nt->flags & NTYPE_ENABLED ) )
		return;

	if( nt->flags & NTYPE_TCP_ENABLED )
	{
		snprintf( buf, 16, "tcp_loop_%hu", nt->tcp->port );
		thread_throw_named( tcp_loop, nt->tcp, 0, buf );
	}

	if( nt->flags & NTYPE_UDP_ENABLED )
	{
		if( nt->flags & NTYPE_UDP_CHECKS )
			fp = &udp_loop_checks;
		else
			fp = &udp_loop_flat;

		for( i = 0; i < nt->udp_count; i++ )
		{
			snprintf( buf, 16, "udp_loop_%hu", nt->udp[i]->port );
			thread_throw_named( fp, nt->udp[i], i, buf );
		}
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
		nt->tcp->fd = tcp_listen( nt->tcp->port, nt->tcp->ip, nt->tcp->back );
		if( nt->tcp->fd < 0 )
			return -1;

		// and the lock for keeping track of current connections
		pthread_mutex_init( &(nt->lock), &(ctl->proc->mtxa) );
	}

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

	return net_startup( ctl->net->relay );
}


void net_shutdown( NET_TYPE *nt )
{
	int i;

	if( !nt || !( nt->flags & NTYPE_ENABLED ) )
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

	net_shutdown( ctl->net->relay );
}


NET_TYPE *net_type_defaults( int type )
{
	NET_TYPE *nt;

	nt              = (NET_TYPE *) allocz( sizeof( NET_TYPE ) );
	nt->tcp         = (NET_PORT *) allocz( sizeof( NET_PORT ) );
	nt->tcp->ip     = INADDR_ANY;
	nt->tcp->back   = DEFAULT_NET_BACKLOG;
	nt->tcp->port   = DEFAULT_RR_PORT;
	nt->tcp->type   = nt;
	nt->flat_parser = &relay_simple;
	nt->prfx_parser = &relay_prefix;
	nt->udp_bind    = INADDR_ANY;
	nt->label       = strdup( "relay" );
	nt->name        = strdup( "relay" );
	nt->flags       = NTYPE_ENABLED|NTYPE_TCP_ENABLED|NTYPE_UDP_ENABLED;

	return nt;
}


NET_CTL *net_config_defaults( void )
{
	NET_CTL *net;

	net              = (NET_CTL *) allocz( sizeof( NET_CTL ) );
	net->dead_time   = NET_DEAD_CONN_TIMER;
	net->rcv_tmout   = NET_RCV_TMOUT;
	net->flush_nsec  = 1000000 * NET_FLUSH_MSEC;
	net->relay       = net_type_defaults( 0 );

	// can't add default target, it's a linked list

	return net;
}


#define ntflag( f )			if( config_bool( av ) ) nt->flags |= NTYPE_##f; else nt->flags &= ~NTYPE_##f




int net_config_line( AVP *av )
{
	struct in_addr ina;
	NET_TYPE *nt;
	int i, tcp;
	char *d;

	nt = ctl->net->relay;

	if( !( d = strchr( av->aptr, '.' ) ) )
	{
		if( attIs( "timeout" ) )
		{
			ctl->net->dead_time = (time_t) atoi( av->vptr );
			debug( "Dead connection timeout set to %d sec.", ctl->net->dead_time );
		}
		else if( attIs( "rcv_tmout" ) )
		{
			ctl->net->rcv_tmout = (unsigned int) atoi( av->vptr );
			debug( "Receive timeout set to %u sec.", ctl->net->rcv_tmout );
		}
		else if( attIs( "flush_msec" ) )
		{
			ctl->net->flush_nsec = 1000000 * atoll( av->vptr );
			if( ctl->net->flush_nsec < 0 )
				ctl->net->flush_nsec = 1000000 * NET_FLUSH_MSEC;
			debug( "Host flush time set to %ld usec.", ctl->net->flush_nsec / 1000 );
		}
		else if( attIs( "prefixList" ) )
		{
			ctl->net->prefix_list = str_copy( av->vptr, av->vlen );
		}
		else if( attIs( "filterList" ) )
		{
			ctl->net->filter_list = str_copy( av->vptr, av->vlen );
		}
		else
			return -1;

		return 0;
	}

	/* then it's tcp or udp */
	d++;

	if( !strncasecmp( av->aptr, "udp.", 4 ) )
		tcp = 0;
	else if( !strncasecmp( av->aptr, "tcp.", 4 ) )
		tcp = 1;
	else
		return -1;

	av->alen -= d - av->aptr;
	av->aptr  = d;

	if( attIs( "enable" ) )
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
	else if( attIs( "checks" ) )
	{
		if( tcp )
			warn( "To disable prefix checks on TCP, set enable 0 on prefixes." );
		else
			ntflag( UDP_CHECKS );
	}
	else if( attIs( "bind" ) )
	{
		inet_aton( av->vptr, &ina );
		if( tcp )
			nt->tcp->ip = ina.s_addr;
		else
			nt->udp_bind = ina.s_addr;
	}
	else if( attIs( "backlog" ) )
	{
		if( tcp )
			nt->tcp->back = (unsigned short) strtoul( av->vptr, NULL, 10 );
		else
			warn( "Cannot set a backlog for a udp connection." );
	}
	else if( attIs( "port" ) || attIs( "ports" ) )
	{
		if( tcp )
			nt->tcp->port = (unsigned short) strtoul( av->vptr, NULL, 10 );
		else
		{
			char *cp;
			WORDS w;

			// work out how many ports we have
			cp = strdup( av->vptr );
			strwords( &w, cp, 0, ',' );
			if( w.wc > 0 )
			{
				nt->udp_count = w.wc;
				nt->udp       = (NET_PORT **) allocz( w.wc * sizeof( NET_PORT * ) );

				debug( "Discovered %d udp ports for %s", w.wc, nt->label );

				for( i = 0; i < w.wc; i++ )
				{
					nt->udp[i]       = (NET_PORT *) allocz( sizeof( NET_PORT ) );
					nt->udp[i]->port = (unsigned short) strtoul( w.wd[i], NULL, 10 );
					nt->udp[i]->type = nt;
				}
			}

			free( cp );
		}
	}
	else
		return -1;

	return 0;
}

#undef ntflag

