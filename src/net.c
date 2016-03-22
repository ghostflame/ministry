/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* net.c - networking setup and config                                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"


// these need init'd, done in net_config_defaults
uint32_t net_masks[33];



int net_ip_check( struct sockaddr_in *sin )
{
#ifdef DEBUG_IP_CHECK
	struct in_addr ina;
	char network[64];
#endif
	uint32_t ip;
	IPNET *i;

	ip = sin->sin_addr.s_addr;

	for( i = ctl->net->ipcheck->list; i; i = i->next )
		if( ( ip & net_masks[i->bits] ) == i->net )
		{
#ifdef DEBUG_IP_CHECK
			ina.s_addr = i->net;
			strcpy( network, inet_ntoa( ina ) );
			debug( "%sing IP %s based on network %s/%hu",
				( i->type == IP_NET_WHITELIST ) ? "Allow" : "Deny",
				inet_ntoa( sin->sin_addr ), network, i->bits );
#endif
			return i->type;
		}

	return ctl->net->ipcheck->deflt;
}






/*
 * Watched sockets
 *
 * This works by thread_throw_network creating a watcher
 * thread.  As there are various lock-up issues with
 * networking, we have a watcher thread that keeps a check
 * on the networking thread.  It watches for the thread to
 * still exist and the network start time to match what it
 * captured early on.
 *
 * If it sees the time since last activity get too high, we
 * assume that the connection has died.  So we kill the
 * thread and close the connection.
 */

void *net_watched_socket( void *arg )
{
	double starttime;
	//pthread_t nt;
	THRD *t;
	HOST *h;

	t = (THRD *) arg;
	h = (HOST *) t->arg;

	// capture the start time - it's sort of a thread id
	starttime = h->started;
	debug( "Connection from %s starts at %.6f", h->net->name, starttime );

	// capture the thread ID of the watched thread
	// when throwing the handler function
	//nt = thread_throw( t->fp, t->arg );
	thread_throw( t->fp, t->arg );

	while( ctl->run_flags & RUN_LOOP )
	{
		// safe because we never destroy host structures
		if( h->started != starttime )
		{
            debug( "Socket has been freed or re-used." );
			break;
		}

		// just use our maintained clock
		if( ( ctl->curr_time.tv_sec - h->last ) > ctl->net->dead_time )
		{
			// cancel that thread
			//pthread_cancel( nt );
			notice( "Connection from host %s timed out.", h->net->name );
            h->net->flags |= HOST_CLOSE;
			break;
		}

		// we are not busy threads around these parts
		usleep( 1000000 );
	}

	debug( "Watcher of thread %lu, exiting." );
	free( t );
	return NULL;
}


void net_disconnect( int *sock, char *name )
{
	if( shutdown( *sock, SHUT_RDWR ) )
		err( "Shutdown error on connection with %s -- %s",
			name, Err );

	close( *sock );
	*sock = -1;
}


void net_close_host( HOST *h )
{
	net_disconnect( &(h->net->sock), h->net->name );
	debug( "Closed connection from host %s.", h->net->name );

	mem_free_host( &h );
}


HOST *net_get_host( int sock, NET_TYPE *type )
{
	struct sockaddr_in from;
	socklen_t sz;
	char buf[32];
	HOST *h;
	int d;

	sz = sizeof( from );

	if( ( d = accept( sock, (struct sockaddr *) &from, &sz ) ) < 0 )
	{
		// broken
		err( "Accept error -- %s", Err );
		return NULL;
	}

	// get a name
	snprintf( buf, 32, "%s:%hu", inet_ntoa( from.sin_addr ),
		ntohs( from.sin_port ) );

	// are we doing blacklisting/whitelisting?
	if( ctl->net->ipcheck->enabled
	 && net_ip_check( &from ) != 0 )
	{
		if( ctl->net->ipcheck->verbose )
			warn( "Denying connection from %s based on ip check.", buf );

		shutdown( d, SHUT_RDWR );
		close( d );
		return NULL;
	}


	h            = mem_new_host( &from );
	h->net->sock = d;
	h->type      = type;
	// should be a unique timestamp
	h->started   = ctl->curr_time.tv_sec;
	h->last      = h->started;

	debug( "Host start time is %ld", h->started );

	return h;
}




NSOCK *net_make_sock( int insz, int outsz, struct sockaddr_in *peer )
{
	NSOCK *ns;

	ns = (NSOCK *) allocz( sizeof( NSOCK ) );

	if( !ns->name )
		ns->name = perm_str( 32 );

	if( peer )
		snprintf( ns->name, 32, "%s:%hu", inet_ntoa( peer->sin_addr ),
			ntohs( peer->sin_port ) );

	ns->peer = peer;

	if( insz )
		ns->in = mem_new_buf( insz );

	if( outsz )
		ns->out = mem_new_buf( outsz );

	// no socket yet
	ns->sock = -1;

	return ns;
}





int net_listen_tcp( unsigned short port, uint32_t ip, int backlog )
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


int net_listen_udp( unsigned short port, uint32_t ip )
{
	struct sockaddr_in sa;
	struct timeval tv;
	int s;

	if( ( s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0 )
	{
		err( "Unable to make udp listen socket -- %s", Err );
		return -1;
	}

	tv.tv_sec  = ctl->net->rcv_tmout;
	tv.tv_usec = 0;

	debug( "Setting receive timeout to %ld.%06ld", tv.tv_sec, tv.tv_usec );

	if( setsockopt( s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof( struct timeval ) ) < 0 )
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
		return -1;
	}

	info( "Bound to udp port %s:%hu for packets.", inet_ntoa( sa.sin_addr ), port );

	return s;
}







int net_startup( NET_TYPE *nt )
{
	int i, j;

	if( !( nt->flags & NTYPE_ENABLED ) )
		return 0;

	if( nt->flags & NTYPE_TCP_ENABLED )
	{
		nt->tcp->sock = net_listen_tcp( nt->tcp->port, nt->tcp->ip, nt->tcp->back );
		if( nt->tcp->sock < 0 )
			return -1;
	}

	if( nt->flags & NTYPE_UDP_ENABLED )
		for( i = 0; i < nt->udp_count; i++ )
		{
			// grab the udp ip variable
			nt->udp[i]->ip   = nt->udp_bind;
			nt->udp[i]->sock = net_listen_udp( nt->udp[i]->port, nt->udp[i]->ip );
			if( nt->udp[i]->sock < 0 )
			{
				if( nt->flags & NTYPE_TCP_ENABLED )
				{
					close( nt->tcp->sock );
					nt->tcp->sock = -1;
				}

				for( j = 0; j < i; j++ )
				{
					close( nt->udp[j]->sock );
					nt->udp[j]->sock = -1;
				}
				return -2;
			}
			debug( "Bound udp port %hu with socket %d",
					nt->udp[i]->port, nt->udp[i]->sock );
		}

	notice( "Started up %s", nt->label );
	return 0;
}



int net_start( void )
{
	IPCHK *c = ctl->net->ipcheck;
	IPNET *n, *list;
	int ret = 0;

	notice( "Starting networking." );

	ret += net_startup( ctl->net->stats );
	ret += net_startup( ctl->net->adder );
	ret += net_startup( ctl->net->gauge );
	ret += net_startup( ctl->net->compat );

	// reverse the ip check list
	list = NULL;
	while( c->list )
	{
		n = c->list;
		c->list = n->next;

		n->next = list;
		list = n;
	}
	c->list = list;

	return ret;
}


void net_shutdown( NET_TYPE *nt )
{
	int i;

	if( !( nt->flags & NTYPE_ENABLED ) )
		return;

	if( ( nt->flags & NTYPE_TCP_ENABLED ) && nt->tcp->sock > -1 )
	{
		close( nt->tcp->sock );
		nt->tcp->sock = -1;
	}

	if( nt->flags & NTYPE_UDP_ENABLED )
		for( i = 0; i < nt->udp_count; i++ )
			if( nt->udp[i]->sock > -1 )
			{
				close( nt->udp[i]->sock );
				nt->udp[i]->sock = -1;
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


NET_TYPE *net_type_defaults( unsigned short port, line_fn *lfn, add_fn *afn, char *label )
{
	NET_TYPE *nt;

	nt            = (NET_TYPE *) allocz( sizeof( NET_TYPE ) );
	nt->tcp       = (NET_PORT *) allocz( sizeof( NET_PORT ) );
	nt->tcp->ip   = INADDR_ANY;
	nt->tcp->back = DEFAULT_NET_BACKLOG;
	nt->tcp->port = port;
	nt->tcp->type = nt;
	nt->parser    = lfn;
	nt->handler   = afn;
	nt->udp_bind  = INADDR_ANY;
	nt->label     = strdup( label );
	nt->flags     = NTYPE_ENABLED|NTYPE_TCP_ENABLED|NTYPE_UDP_ENABLED;

	return nt;
}



NET_CTL *net_config_defaults( void )
{
	NET_CTL *net;
	int i;

	net            = (NET_CTL *) allocz( sizeof( NET_CTL ) );
	net->dead_time = NET_DEAD_CONN_TIMER;
	net->rcv_tmout = NET_RCV_TMOUT;
	net->reconn    = 1000 * NET_RECONN_MSEC;
	net->io_usec   = 1000 * NET_IO_MSEC;
	net->max_bufs  = IO_MAX_WAITING;

	net->stats     = net_type_defaults( DEFAULT_STATS_PORT,  &data_line_ministry, &data_point_stats,  "ministry stats socket" );
	net->adder     = net_type_defaults( DEFAULT_ADDER_PORT,  &data_line_ministry, &data_point_adder,  "ministry adder socket" );
	net->gauge     = net_type_defaults( DEFAULT_GAUGE_PORT,  &data_line_ministry, &data_point_gauge,  "ministry gauge socket" );
	net->compat    = net_type_defaults( DEFAULT_COMPAT_PORT, &data_line_compat,   NULL,               "statsd compat socket"  );

	// ip checking
	net->ipcheck   = (IPCHK *) allocz( sizeof( IPCHK ) );

	// can't add default target, it's a linked list

	// init our net_masks
	net_masks[0] = 0;
	for( i = 1; i < 33; i++ )
		net_masks[i] = net_masks[i-1] | ( 1 << ( i - 1 ) );

	return net;
}


#define ntflag( f )			if( atoi( av->val ) ) nt->flags |= NTYPE_##f; else nt->flags &= ~NTYPE_##f


int net_add_list_member( char *str, int len, uint16_t type )
{
	struct in_addr ina;
	IPNET *ip;
	int bits;
	char *sl;

	if( ( sl = memchr( str, '/', len ) ) )
	{
		*sl++ = '\0';
		bits = atoi( sl );
	}
	else
		bits = 32;

	if( bits < 0 || bits > 32 )
	{
		err( "Invalid network bits %d", bits );
		return -1;
	}

	// did that look OK?
	if( !inet_aton( str, &ina ) )
	{
		err( "Invalid ip address %s", str );
		return -1;
	}

	ip = (IPNET *) allocz( sizeof( IPNET ) );
	ip->bits = bits;
	ip->type = type;
	// and grab the network from that
	// doing this makes 172.16.10.10/16 work as a network
	ip->net = ina.s_addr & net_masks[bits];

	// add it into the list
	// this needs reversing at net start
	ip->next = ctl->net->ipcheck->list;
	ctl->net->ipcheck->list = ip;

	return 0;
}



int net_config_line( AVP *av )
{
	struct in_addr ina;
	char *d, *p, *cp;
	NET_TYPE *nt;
	int i, tcp;
	TARGET *t;
	WORDS *w;


	if( !( d = strchr( av->att, '.' ) ) )
	{
		if( attIs( "timeout" ) )
		{
			ctl->net->dead_time = (time_t) atoi( av->val );
			debug( "Dead connection timeout set to %d sec.", ctl->net->dead_time );
		}
		else if( attIs( "rcv_tmout" ) )
		{
			ctl->net->rcv_tmout = (unsigned int) atoi( av->val );
			debug( "Receive timeout set to %u sec.", ctl->net->rcv_tmout );
		}
		else if( attIs( "reconn_msec" ) )
		{
			ctl->net->reconn = 1000 * atoi( av->val );
			if( ctl->net->reconn <= 0 )
				ctl->net->reconn = 1000 * NET_RECONN_MSEC;
			debug( "Reconnect time set to %d usec.", ctl->net->reconn );
		}
		else if( attIs( "io_msec" ) )
		{
			ctl->net->io_usec = 1000 * atoi( av->val );
			if( ctl->net->io_usec <= 0 )
				ctl->net->io_usec = 1000 * NET_IO_MSEC;
			debug( "Io loop time set to %d usec.", ctl->net->io_usec );
		}
		else if( attIs( "max_waiting" ) )
		{
			ctl->net->max_bufs = atoi( av->val );
			if( ctl->net->max_bufs <= 0 )
				ctl->net->max_bufs = IO_MAX_WAITING;
			debug( "Max waiting buffers set to %d.", ctl->net->max_bufs );
		}
		else if( attIs( "target" ) || attIs( "targets" ) )
		{
			w  = (WORDS *) allocz( sizeof( WORDS ) );
			cp = strdup( av->val );
			strwords( w, cp, 0, ',' );

			for( i = 0; i < w->wc; i++ )
			{
				t = (TARGET *) allocz( sizeof( TARGET ) );

				if( ( p = memchr( w->wd[i], ':', w->len[i] ) ) )
				{
					t->host = str_dup( w->wd[i], p - w->wd[i] );
					t->port = (uint16_t) strtoul( p + 1, NULL, 10 );
				}
				else
				{
					t->host = str_dup( w->wd[i], w->len[i] );
					t->port = DEFAULT_TARGET_PORT;
				}

				t->next = ctl->net->targets;
				ctl->net->targets = t;
				ctl->net->tcount++;
			}

			free( cp );
			free( w );
		}
		else
			return -1;

		return 0;
	}

	/* then it's data., statsd. or adder. (or ipcheck) */
	p = d + 1;

	// the names changed, so support both
	if( !strncasecmp( av->att, "stats.", 6 ) || !strncasecmp( av->att, "data.", 5 ) )
		nt = ctl->net->stats;
	else if( !strncasecmp( av->att, "adder.", 6 ) )
		nt = ctl->net->adder;
	else if( !strncasecmp( av->att, "gauge.", 6 ) || !strncasecmp( av->att, "guage.", 6 ) )
		nt = ctl->net->gauge;
	else if( !strncasecmp( av->att, "compat.", 7 ) || !strncasecmp( av->att, "statsd.", 7 ) )
		nt = ctl->net->compat;
	else if( !strncasecmp( av->att, "ipcheck.", 8 ) )
	{
		if( !strcasecmp( p, "enable" ) )
			ctl->net->ipcheck->enabled = atoi( av->val );
		else if( !strcasecmp( p, "verbose" ) )
			ctl->net->ipcheck->verbose = atoi( av->val );
		else if( !strcasecmp( p, "whitelist" ) )
			return net_add_list_member( av->val, av->vlen, IP_NET_WHITELIST );
		else if( !strcasecmp( p, "blacklist" ) )
			return net_add_list_member( av->val, av->vlen, IP_NET_BLACKLIST );
		else if( !strcasecmp( p, "drop_unknown" ) )
			ctl->net->ipcheck->deflt = 1;

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
			nt->tcp->back = (unsigned short) strtoul( av->val, NULL, 10 );
		else
			warn( "Cannot set a backlog for a udp connection." );
	}
	else if( !strcasecmp( d, "port" ) || !strcasecmp( d, "ports" ) )
	{
		if( tcp )
			nt->tcp->port = (unsigned short) strtoul( av->val, NULL, 10 );
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

