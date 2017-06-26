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



// look up an IP in a list
static inline IPNET *net_ip_lookup( uint32_t ip, IPCHK *in )
{
#ifdef DEBUG_IPCHK
	struct in_addr ina;
#endif
	IPNET *n;

#ifdef DEBUG_IPCHK
	ina.s_addr = ip;
#endif
	debug_ipck( "Checking ip %s for %s", inet_ntoa( ina ), in->name );

	// try the hash
	for( n = in->ips[ip % in->hashsz]; n; n = n->next )
	{
#ifdef DEBUG_IPCHK
		ina.s_addr = n->ipnet;
#endif
		debug_ipck( "Trying %s", inet_ntoa( ina ) );
		if( ip == n->ipnet )
		{
			debug_ipck( "Match on %s", inet_ntoa( ina ) );
			return n;
		}
	}

	// try the networks
	for( n = in->nets; n; n = n->next )
	{
#ifdef DEBUG_IPCHK
		ina.s_addr = n->ipnet;
#endif
		debug_ipck( "Trying %s/%hu", inet_ntoa( ina ), n->bits );
		if( ( ip & net_masks[n->bits] ) == n->ipnet )
		{
			debug_ipck( "Match on %s/%hu", inet_ntoa( ina ), n->bits );
			return n;
		}
	}

	return NULL;
}


// 0 means OK, 1 means drop
int net_ip_check( struct sockaddr_in *sin )
{
	IPNET *i;

	if( !ctl->net->iplist->enable
	 || !ctl->net->iplist->total )
		return 0;

	if( !( i = net_ip_lookup( sin->sin_addr.s_addr, ctl->net->iplist ) ) )
		return ctl->net->iplist->drop;

	if( i->act == NET_IP_BLACKLIST )
		return 1;

	return 0;
}


HPRFX *net_prefix_check( struct sockaddr_in *sin )
{
	IPNET *i = NULL;

	if( ctl->net->prefix->enable
	 && ctl->net->prefix->total )
		i = net_ip_lookup( sin->sin_addr.s_addr, ctl->net->prefix );

	return ( i ) ? i->prefix : NULL;
}



// set prefix data on a host
int net_set_host_prefix( HOST *h, HPRFX *pr )
{
	// not a botch - simplifies other callers
	if( !pr )
		return 0;

	// set that prefix
	h->prefix = pr;

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
	memcpy( h->workbuf, pr->pstr, pr->plen );
	h->workbuf[pr->plen] = '\0';
	h->plen = pr->plen;

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
		return net_set_host_prefix( h, net_prefix_check( h->peer ) );

	return 0;
}



NSOCK *net_make_sock( int insz, int outsz, struct sockaddr_in *peer )
{
	NSOCK *ns;

	if( !( ns = (NSOCK *) allocz( sizeof( NSOCK ) ) ) )
	{
		fatal( "Could not allocate new nsock." );
		return NULL;
	}

	if( !ns->name )
	{
		ns->name = perm_str( 32 );

		snprintf( ns->name, 32, "%s:%hu", inet_ntoa( peer->sin_addr ),
			ntohs( peer->sin_port ) );
	}

	// copy the peer contents
	ns->peer = *peer;

	if( insz )
		ns->in = mem_new_buf( insz );

	if( outsz )
		ns->out = mem_new_buf( outsz );

	// no socket yet
	ns->sock = -1;

	return ns;
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
		if( nt->flags & NTYPE_UDP_CHECKS )
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
		nt->tcp->sock = tcp_listen( nt->tcp->port, nt->tcp->ip, nt->tcp->back );
		if( nt->tcp->sock < 0 )
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
			nt->udp[i]->sock = udp_listen( nt->udp[i]->port, nt->udp[i]->ip );
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


void __net_rule_explain_list( IPNET *list, int show_prfx )
{
	struct in_addr ina;
	IPNET *ip;

	for( ip = list; ip; ip = ip->next )
	{
		ina.s_addr = ip->ipnet;
		info( "   %-6s    %s/%hu    %s",
			( ip->act == NET_IP_WHITELIST ) ? ( ( show_prfx ) ? "Prefix" : "Allow" ) : "Deny",
			inet_ntoa( ina ), ip->bits,
			( show_prfx ) ? ip->prefix->pstr : "" );
	}
}

int net_start( void )
{
	int ret = 0;
	IPCHK *ipc;
	int i;

	// create our token hash table
	token_init( );

	// reverse the ip net lists
	ctl->net->iplist->nets = (IPNET *) mem_reverse_list( ctl->net->iplist->nets );
	ctl->net->prefix->nets = (IPNET *) mem_reverse_list( ctl->net->prefix->nets );

	ipc = ctl->net->iplist;
	if( ipc->enable && ipc->verbose )
	{
		info( "Network ipcheck config order:" );

		for( i = 0; i < ipc->hashsz; i++ )
			__net_rule_explain_list( ipc->ips[i], 0 );

		__net_rule_explain_list( ipc->nets, 0 );

		info( "   %-6s    %s", ( ipc->drop ) ? "Deny" : "Allow", "Default" );
	}

	ipc = ctl->net->prefix;
	if( ipc->enable && ipc->verbose )
	{
		info( "Prefixing config order:" );

		for( i = 0; i < ipc->hashsz; i++ )
			__net_rule_explain_list( ipc->ips[i], 1 );

		__net_rule_explain_list( ipc->nets, 1 );
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

	if( ( nt->flags & NTYPE_TCP_ENABLED ) && nt->tcp->sock > -1 )
	{
		close( nt->tcp->sock );
		nt->tcp->sock = -1;
		pthread_mutex_destroy( &(nt->lock) );
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



IPCHK *__net_get_ipcheck( int hashsz, char *name )
{
	IPCHK *ipc;

	// ip checking
	ipc              = (IPCHK *) allocz( sizeof( IPCHK ) );
	ipc->ips         = (IPNET **) allocz( NET_IP_HASHSZ * sizeof( IPNET * ) );
	ipc->hashsz      = NET_IP_HASHSZ;
	ipc->name        = str_dup( name, 0 );

	return ipc;
}


NET_CTL *net_config_defaults( void )
{
	NET_CTL *net;
	char *errbuf;
	int i, rv;

	net              = (NET_CTL *) allocz( sizeof( NET_CTL ) );
	net->dead_time   = NET_DEAD_CONN_TIMER;
	net->rcv_tmout   = NET_RCV_TMOUT;
	net->reconn      = 1000 * NET_RECONN_MSEC;
	net->io_usec     = 1000 * NET_IO_MSEC;
	net->max_bufs    = IO_MAX_WAITING;

	net->stats       = net_type_defaults( DATA_TYPE_STATS );
	net->adder       = net_type_defaults( DATA_TYPE_ADDER );
	net->gauge       = net_type_defaults( DATA_TYPE_GAUGE );
	net->compat      = net_type_defaults( DATA_TYPE_COMPAT );

	if( ( rv = regcomp( &(net->ipregex), NET_IP_REGEX_STR, REG_EXTENDED|REG_ICASE ) ) )
	{
		errbuf = (char *) allocz( 2048 );

		regerror( rv, &(net->ipregex), errbuf, 2048 );
		fatal( "Cannot make IP address regex -- %s", errbuf );

		free( errbuf );
		return NULL;
	}
	if( !( net->iplist = __net_get_ipcheck( NET_IP_HASHSZ, "ipcheck" ) ) )
		return NULL;

	if( !( net->prefix = __net_get_ipcheck( NET_IP_HASHSZ, "prefix" ) ) )
		return NULL;

	// create our tokens structure
	net->tokens = token_setup( );

	// can't add default target, it's a linked list

	// init our net_masks
	net_masks[0] = 0;
	for( i = 1; i < 33; i++ )
		net_masks[i] = net_masks[i-1] | ( 1 << ( i - 1 ) );

	return net;
}


#define ntflag( f )			if( config_bool( av ) ) nt->flags |= NTYPE_##f; else nt->flags &= ~NTYPE_##f


int net_lookup_host( char *host, struct sockaddr_in *res )
{
	struct addrinfo *ap, *ai = NULL;

	// try the lookup
	if( getaddrinfo( host, NULL, NULL, &ai ) )
	{
		err( "Could not look up host %s -- %s", host, Err );
		return -1;
	}

	// we get anything?
	if( !ai )
	{
		err( "Found nothing when looking up host %s", host );
		freeaddrinfo( ai );
		return -1;
	}

	// find an AF_INET answer - we don't do ipv6 yet
	for( ap = ai; ap; ap = ap->ai_next )
		if( ap->ai_family == AF_INET )
			break;

	// none?
	if( !ap )
	{
		err( "Could not find an IPv4 answer for address %s", host );
		freeaddrinfo( ai );
		return -1;
	}

	// copy the first result out
	res->sin_family = ap->ai_family;
	res->sin_addr   = ((struct sockaddr_in *) ap->ai_addr)->sin_addr;

	freeaddrinfo( ai );
	return 0;
}




IPNET *__net_parse_hostspec( char *str )
{
	struct sockaddr_in sa;
	regmatch_t rm[7];
	IPNET *ip = NULL;
	int bits = -1;

	// does it match?
	if( !regexec( &(ctl->net->ipregex), str, 7, rm, 0 ) )
	{
		// cap the IP address - might stomp on the /
		// or just be the end of the string (what was the newline once)
		str[rm[1].rm_eo] = '\0';

		// do we have some bits?
		if( rm[6].rm_so > -1 )
			bits = atoi( str + rm[6].rm_so );
		else
			bits = 32;

		if( bits < 0 || bits > 32 )
		{
			err( "Invalid network bits %d", bits );
			return NULL;
		}

		// did that look OK?
		if( !inet_aton( str, &(sa.sin_addr) ) )
		{
			err( "Invalid ip address %s", str );
			return NULL;
		}
	}
	// try it as a hostname
	else if( !net_lookup_host( str, &sa ) )
		bits = 32;

	// did we get something?
	if( bits > -1 )
	{
		ip = (IPNET *) allocz( sizeof( IPNET ) );
		ip->bits = bits;

		// and grab the network
		// doing this makes 172.16.10.10/16 work as a network
		ip->ipnet = sa.sin_addr.s_addr & net_masks[bits];
	}

	return ip;
}








int __net_hprfx_dedupe( IPNET *cur, IPNET *prv )
{
	HPRFX *h, *p;

	h = cur->prefix;
	p = prv->prefix;

	// do they match?
	if( h->plen == p->plen && !memcmp( h->pstr, p->pstr, p->plen ) )
	{
		warn( "Duplicate prefix entries: %s vs %s, ignoring %s",
			h->confstr, p->confstr, h->confstr );

		// tidy up
		free( h->confstr );
		free( h->pstr );
		free( h );
		free( cur );

		return 0;
	}

	err( "Previous prefix %s -> %s", p->confstr, p->pstr );
	err( "New prefix %s -> %s", h->confstr, h->pstr );
	err( "Duplicate prefix mismatch." );
	return -1;
}


int __net_ipcheck_insert( IPNET *ip, IPCHK *into )
{
	struct in_addr ina;
	IPNET *i = NULL;
	uint32_t hval;

	ina.s_addr = ip->ipnet;

	// check for duplicates
	if( ip->bits == 32 )
	{
		hval = ip->ipnet % into->hashsz;

		for( i = into->ips[hval]; i; i = i->next )
			if( ip->ipnet == i->ipnet )
			 	break;

		if( !i )
		{
			ip->next = into->ips[hval];
			into->ips[hval] = ip;
			if( ip->prefix )
				debug( "Added ip prefix %s --> %s", inet_ntoa( ina ), ip->prefix->pstr );
			else
			{
				ina.s_addr = ip->ipnet;
				debug( "Added ip rule %s --> %s", inet_ntoa( ina ),
					( ip->act == NET_IP_WHITELIST ) ? "allow" : "drop" );
			}
			into->total++;
			return 0;
		}
	}
	else
	{
		for( i = into->nets; i; i = i->next )
			if( ip->ipnet == i->ipnet && ip->bits == i->bits )
				break;

		if( !i )
		{
			ip->next = into->nets;
			into->nets = ip;
			if( ip->prefix )
				debug( "Added net prefix %s/%hu --> %s", inet_ntoa( ina ), ip->bits,
					ip->prefix->pstr );
			else
			{
				ina.s_addr = ip->ipnet;
				debug( "Added net rule %s/%hu --> %s", inet_ntoa( ina ), ip->bits,
					( ip->act == NET_IP_WHITELIST ) ? "allow" : "drop" );
			}
			into->total++;
			return 0;
		}
	}

	// OK, we have a match

	if( ip->prefix )
		return __net_hprfx_dedupe( ip, i );

	err( "Duplicate IP check rule: %s/%hu",
		inet_ntoa( ina ), ip->bits );
	free( ip );

	return -1;
}




// parse a prefix line:  <hostspec> <prefix>
int net_config_prefix( char *line, int len )
{
	int plen, hlen, adddot = 0;
	char *prfx;
	IPNET *ip;
	HPRFX *h;

	if( !( prfx = memchr( line, ' ', len ) ) )
	{
		err( "Invalid prefix statement: %s", line );
		return -1;
	}

	hlen = prfx - line;

	while( *prfx && isspace( *prfx ) )
		*prfx++ = '\0';

	if( !*prfx )
	{
		err( "Empty prefix string!" );
		return -1;
	}

	plen = len - ( prfx - line );

	if( plen > 1023 )
	{
		err( "Prefix string too line: max is 1023 bytes." );
		return -1;
	}

	// did we end with a dot?
	if( line[len - 1] != '.' )
		adddot = 1;

	// try parsing the string
	if( !( ip = __net_parse_hostspec( line ) ) )
		return -1;

	h = ( ip->prefix = (HPRFX *) allocz( sizeof( HPRFX ) ) );

	// copy the string, add a dot if necessary
	h->pstr = str_copy( prfx, plen + adddot );
	if( adddot )
	{
		h->pstr[plen]   = '.';
		h->pstr[plen+1] = '\0';
	}
	h->plen = plen + adddot;

	// copy the host config
	h->confstr = str_copy( line, hlen );

	// and insert it
	return __net_ipcheck_insert( ip, ctl->net->prefix );
}



// add a whitelist/blacklist member
int net_add_list_member( char *str, int len, uint16_t act )
{
	IPNET *ip;

	// make the IP check structure
	if( !( ip = __net_parse_hostspec( str ) ) )
		return -1;

	ip->act = act;

	// and insert it
	return __net_ipcheck_insert( ip, ctl->net->iplist );
}



int net_config_line( AVP *av )
{
	NET_CTL *n = ctl->net;
	struct in_addr ina;
	char *d, *p, *cp;
	NET_TYPE *nt;
	int i, tcp;
	int64_t v;
	TARGET *t;
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
		else if( attIs( "reconnMsec" ) )
		{
			av_intk( n->reconn );
			if( n->reconn <= 0 )
				n->reconn = 1000 * NET_RECONN_MSEC;
			debug( "Reconnect time set to %d usec.", n->reconn );
		}
		else if( attIs( "ioMsec" ) )
		{
			av_intk( n->io_usec );
			if( n->io_usec <= 0 )
				n->io_usec = 1000 * NET_IO_MSEC;
			debug( "Io loop time set to %d usec.", n->io_usec );
		}
		else if( attIs( "maxWaiting" ) )
		{
			warn( "Net config max_waiting is deprecated - use [Target] : max_waiting." );
			av_int( n->max_bufs );
			if( n->max_bufs <= 0 )
				n->max_bufs = IO_MAX_WAITING;
		}
		else if( attIs( "prefix" ) )
		{
			err( "Prefix singleton is deprecated: use prefix.set." );
		}
		else if( attIs( "target" ) || attIs( "targets" ) )
		{
			warn( "WARNING: config item %s is deprecated, use [Target] section.", av->att );
			return 0;

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

				t->next = n->targets;
				n->targets = t;
				n->tcount++;
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
		nt = n->stats;
	else if( !strncasecmp( av->att, "adder.", 6 ) )
		nt = n->adder;
	else if( !strncasecmp( av->att, "gauge.", 6 ) || !strncasecmp( av->att, "guage.", 6 ) )
		nt = n->gauge;
	else if( !strncasecmp( av->att, "compat.", 7 ) || !strncasecmp( av->att, "statsd.", 7 ) )
		nt = n->compat;
	else if( !strncasecmp( av->att, "prefix.", 7 ) )
	{
		if( !strcasecmp( p, "enable" ) )
			n->prefix->enable = config_bool( av );
		else if( !strcasecmp( p, "verbose" ) )
			n->prefix->verbose = config_bool( av );
		else if( !strcasecmp( p, "set" ) || !strcasecmp( p, "add" ) )
		{
			// this is messy
			return net_config_prefix( av->val, av->vlen );
		}
		else
			return -1;

		return 0;
	}
	else if( !strncasecmp( av->att, "ipcheck.", 8 ) )
	{
		if( !strcasecmp( p, "enable" ) )
			n->iplist->enable = config_bool( av );
		else if( !strcasecmp( p, "verbose" ) )
			n->iplist->verbose = config_bool( av );
		else if( !strcasecmp( p, "dropUnknown" ) || !strcasecmp( p, "drop" ) )
			n->iplist->drop = config_bool( av );
		else if( !strcasecmp( p, "whitelist" ) )
			return net_add_list_member( av->val, av->vlen, NET_IP_WHITELIST );
		else if( !strcasecmp( p, "blacklist" ) )
			return net_add_list_member( av->val, av->vlen, NET_IP_BLACKLIST );
		else
			return -1;

		return 0;
	}
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

