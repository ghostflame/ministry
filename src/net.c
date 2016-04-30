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


static inline HPRFX *net_prefix_ip_lookup( uint32_t ip )
{
	HPRFX *p;

	for( p = ctl->net->prefix->ips[ip % HPRFX_HASHSZ]; p; p = p->next )
	{
		debug( "Checking incoming %08x vs rule %08x", ip, p->ip );
		if( ip == p->ip )
			return p;
	}

	return NULL;
}



HPRFX *net_prefix_check( struct sockaddr_in *sin )
{
	uint32_t ip;
	HPRFX *p;

	ip = sin->sin_addr.s_addr;
	debug( "Calling IP is %08x", ip );

	// direct hash check?
	if( ( p = net_prefix_ip_lookup( ip ) ) )
		return p;

	// networks check
	for( p = ctl->net->prefix->nets; p; p = p->next )
		if( ( ip & net_masks[p->net->bits] ) == p->net->net )
			return p;

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

	// give us a moment
	usleep( 10000 );
	mem_free_host( &h );
}


HOST *net_get_host( int sock, NET_TYPE *type )
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
		return NULL;
	}

	// are we doing blacklisting/whitelisting?
	if( ctl->net->ipcheck->enabled
	 && net_ip_check( &from ) != 0 )
	{
		if( ctl->net->ipcheck->verbose )
			warn( "Denying connection from %s:%hu based on ip check.",
				inet_ntoa( from.sin_addr ), ntohs( from.sin_port ) );

		shutdown( d, SHUT_RDWR );
		close( d );
		return NULL;
	}

	if( !( h = mem_new_host( &from ) ) )
		fatal( "Could not allocate new host." );

	h->net->sock = d;
	h->type      = type;

	// assume type-based handler functions
	h->parser    = type->flat_parser;

	// do we need to override those?
	if( ctl->net->prefix->total > 0
	 && ( h->prefix = net_prefix_check( &from ) ) )
	{
		// change the parser function to one that does prefixing
		h->parser = type->prfx_parser;

		// and copy the prefix into the workbuf
		if( !h->workbuf && !( h->workbuf = (char *) allocz( HPRFX_BUFSZ ) ) )
		{
			mem_free_host( &h );
			fatal( "Could not allocate host work buffer" );
			return NULL;
		}

		// and make a copy of the prefix for this host
		memcpy( h->workbuf, h->prefix->pstr, h->prefix->plen );
		h->plen = h->prefix->plen;

		// set the max line we like and the target to copy to
		h->lmax = HPRFX_BUFSZ - h->plen - 1;
		h->ltarget = h->workbuf + h->plen;

		info( "Connection from %s:%hu gets prefix %s",
				inet_ntoa( from.sin_addr ), ntohs( from.sin_port ),
				h->workbuf );
	}
	else
		debug( "Connection from %s:%hu gets no prefix.",
				inet_ntoa( from.sin_addr ), ntohs( from.sin_port ) );

	return h;
}




NSOCK *net_make_sock( int insz, int outsz, struct sockaddr_in *peer )
{
	NSOCK *ns;

	if( !( ns = (NSOCK *) allocz( sizeof( NSOCK ) ) ) )
		fatal( "Could not allocate new nsock." );

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
	HPRFXS *p = ctl->net->prefix;
	HPRFX *h, *hlist;
	IPNET *n, *nlist;
	int ret = 0;

	// reverse the ip check list
	nlist = NULL;
	while( c->list )
	{
		n = c->list;
		c->list = n->next;

		n->next = nlist;
		nlist = n;
	}
	c->list = nlist;

	// reverse the network prefixes
	hlist = NULL;
	while( p->nets )
	{
		h = p->nets;
		p->nets = h->next;

		h->next = hlist;
		hlist = h;
	}
	p->nets = hlist;


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
	const struct data_type_params *d = data_type_defns + type;
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
	nt->label       = strdup( d->sock );
	nt->flags       = NTYPE_ENABLED|NTYPE_TCP_ENABLED|NTYPE_UDP_ENABLED;

	return nt;
}




NET_CTL *net_config_defaults( void )
{
	NET_CTL *net;
	int i;

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

	// ip checking
	net->ipcheck     = (IPCHK *) allocz( sizeof( IPCHK ) );

	// prefix checking
	net->prefix      = (HPRFXS *) allocz( sizeof( HPRFXS ) );
	net->prefix->ips = (HPRFX **) allocz( HPRFX_HASHSZ * sizeof( HPRFX * ) );

	// can't add default target, it's a linked list

	// init our net_masks
	net_masks[0] = 0;
	for( i = 1; i < 33; i++ )
		net_masks[i] = net_masks[i-1] | ( 1 << ( i - 1 ) );

	return net;
}


#define ntflag( f )			if( atoi( av->val ) ) nt->flags |= NTYPE_##f; else nt->flags &= ~NTYPE_##f


static regex_t *__net_ip_regex = NULL;


IPNET *__net_parse_network( char *str, int *saw_error )
{
	struct in_addr ina;
	regmatch_t rm[7];
	int bits, rv;
	IPNET *ip;

	*saw_error = 0;

	if( !__net_ip_regex )
	{
		__net_ip_regex = (regex_t *) allocz( sizeof( regex_t ) );

		if( ( rv = regcomp( __net_ip_regex, NET_IP_REGEX_STR, REG_EXTENDED|REG_ICASE ) ) )
		{
			char *errbuf = (char *) allocz( 2048 );

			regerror( rv, __net_ip_regex, errbuf, 2048 );
			fatal( "Cannot make IP address regex -- %s", errbuf );
			free( errbuf );

			*saw_error = 1;
			return NULL;
		}
	}

	// was it a match?
	if( regexec( __net_ip_regex, str, 7, rm, 0 ) )
		return NULL;

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
		*saw_error = 1;
		return NULL;
	}

	// did that look OK?
	if( !inet_aton( str, &ina ) )
	{
		err( "Invalid ip address %s", str );
		*saw_error = 1;
		return NULL;
	}

	ip = (IPNET *) allocz( sizeof( IPNET ) );
	ip->bits = bits;

	// and grab the network from that
	// doing this makes 172.16.10.10/16 work as a network
	ip->net = ina.s_addr & net_masks[bits];

	return ip;
}




int net_add_list_member( char *str, int len, uint16_t type )
{
	IPNET *ip;
	int er;

	if( !( ip = __net_parse_network( str, &er ) ) )
		return -1;

	ip->type = type;

	// add it into the list
	// this needs reversing at net start
	ip->next = ctl->net->ipcheck->list;
	ctl->net->ipcheck->list = ip;

	return 0;
}


int net_lookup_host( char *host, struct sockaddr_in *res )
{
	struct addrinfo *ap, *ai = NULL;

	// try the lookup
	if( getaddrinfo( host, NULL, NULL, &ai ) || !ai )
	{
		err( "Could not look up host %s -- %s", host, Err );
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



int __net_hprfx_dedupe( HPRFX *h, HPRFX *prv )
{
	// do they match?
	if( h->plen == prv->plen && !memcmp( h->pstr, prv->pstr, h->plen ) )
	{
		warn( "Duplicate prefix entries: %s vs %s, ignoring %s",
			h->confstr, prv->confstr, h->confstr );

		// tidy up
		free( h->confstr );
		free( h->pstr );
		free( h->net );
		free( h );

		return 0;
	}

	err( "Previous prefix %s -> %s", prv->confstr, prv->pstr );
	err( "New prefix %s -> %s", h->confstr, h->pstr );
	err( "Duplicate prefix mismatch." );
	return -1;
}



int __net_hprfx_insert( HPRFX *h )
{
	uint32_t hval;
	HPRFX *p;

	if( ( p = net_prefix_ip_lookup( h->ip ) ) )
		return __net_hprfx_dedupe( h, p );

	// OK, a new one
	hval = h->ip % HPRFX_HASHSZ;

	h->next = ctl->net->prefix->ips[hval];
	ctl->net->prefix->ips[hval] = h;
	ctl->net->prefix->total++;
	debug( "Added ip prefix %s --> %s", h->confstr, h->pstr );

	return 0;
}





// parse a prefix line:  <hostspec> <prefix>
int net_config_prefix( char *line, int len )
{
	int plen, hlen, er, adddot = 0;
	struct sockaddr_in sa;
	HPRFX *h, *p;
	char *prfx;

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

	h = (HPRFX *) allocz( sizeof( HPRFX ) );

	// copy the string, add a dot if necessary
	h->pstr = str_copy( prfx, plen + adddot );
	if( adddot )
		h->pstr[plen] = '.';
	h->plen = plen + adddot;

	// copy the host config
	h->confstr = str_copy( line, hlen );

	// see if it's an IP or net/bits network
	if( ( h->net = __net_parse_network( line, &er ) ) )
	{
		if( h->net->bits == 32 )
		{
			// IP address - put it in the hash
			h->ip = h->net->net;

			return __net_hprfx_insert( h );
		}

		// dupe check - same net/bits
		for( p = ctl->net->prefix->nets; p; p = p->next )
			if( p->net->net == h->net->net && p->net->bits == h->net->bits )
				return __net_hprfx_dedupe( h, p );

		// these get reversed to restore the config order
		h->next = ctl->net->prefix->nets;
		ctl->net->prefix->nets = h;
		ctl->net->prefix->total++;
		debug( "Added net prefix %s --> %s", h->confstr, h->pstr );

		return 0;
	}

	// was there a problem?
	if( er )
		return -1;

	// OK, it's a hostname
	if( net_lookup_host( line, &sa ) )
		return -1;

	// grab the IP
	h->ip = sa.sin_addr.s_addr;

	// and put it in the ips hash
	return __net_hprfx_insert( h );
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
		else if( attIs( "prefix" ) )
		{
			// these are messy
			return net_config_prefix( av->val, av->vlen );
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

