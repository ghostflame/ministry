/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* udp.c - handles connectionless data receiving                           *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"



// we don't have a way to handle partial lines received over
// UDP without keeping a cache of keep buffers for every IP address
// seen.  nc will certainly break lines across packets and if your
// app does the same, we will likely screw up your data.  But that's
// UDP for you.
//
// An IP based cache is a DoS vulnerability - we could be supplied
// with partial lines from a huge variety of source addresses and
// balloon our memory trying to keep up.  So for now, a simpler
// view is necessary



static inline HOST *udp_get_phost( NET_PORT *n, IPNET *ipn )
{
	uint64_t hval;
	HOST *h;

	hval = ((uint64_t) ipn) % n->phsz;

	for( h = n->phosts[hval]; h; h = h->next )
		if( h->ipn == ipn )
			return h;

	// this should never happen!
	warn( "Could not find prefix %p : %s", ipn, ipn->name );
	// this blows us up
	return NULL;
}


void udp_add_phost( void *arg, IPNET *ip )
{
	NET_PORT *n = (NET_PORT *) arg;
	struct sockaddr_in sa;
	uint64_t hval;
	HOST *h;

	// don't do this for empty prefixes
	if( !ip->tlen )
		return;

	hval = ((uint64_t) ip) % n->phsz;

	sa.sin_addr.s_addr = ip->net;
	sa.sin_port        = htons( ip->bits );

	h = mem_new_host( &sa, 0 );
	h->type = n->type;

	// and set it up
	if( net_set_host_prefix( h, ip ) != 0 )
	{
		err( "Could not set host prefix for %p : %s", ip, ip->name );
		return;
	}

	// add it into the hash
	h->next = n->phosts[hval];
	n->phosts[hval] = h;
}



void udp_loop_checks( THRD *t )
{
	struct sockaddr_in sa;
	struct sockaddr *sp;
	socklen_t sl;
	NET_PORT *n;
	NET_PFX *p;
	IPNET *ipn;
	int sz, rb;
	IOBUF *ib;
	HOST *h;
	BUF *b;

	n = (NET_PORT *) t->arg;

	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = n->ip;
	sa.sin_port = htons( n->port );

	// make some space for hosts
	n->phosts = (HOST **) allocz( NET_IP_HASHSZ * sizeof( HOST * ) );
	n->phsz = NET_IP_HASHSZ;

	h  = mem_new_host( &sa, NET_BUF_SZ );
	ib = h->net->in;
	b  = ib->bf;

	h->type = n->type;

	// for now we don't do prefixing or tokens on UDP
	net_set_host_parser( h, 0, 0 );

	// prepopulate the hash table with prefixes
	for( p = _net->prefix; p; p = p->next )
		iplist_call_data( p->list, &udp_add_phost, n );

	loop_mark_start( "udp" );

	// we need to make room for a newline and a null for safety
	sz = b->sz - 2;
	// and let's not do constant casting
	sp = (struct sockaddr *) h->peer;

	while( RUNNING( ) )
	{
		// get a packet, set the from
		sl = sizeof( struct sockaddr_in );
		if( ( rb = recvfrom( n->fd, b->buf, sz, 0, sp, &sl ) ) < 0 )
		{
			if( errno == EINTR || errno == EAGAIN )
				continue;

			++(n->errors.count);
			err( "Recvfrom error -- %s", Err );
			loop_end( "receive error on udp socket" );
			break;
		}
		else if( !rb )
			continue;

		b->len = rb;

		// do IP whitelist/blacklist check
		if( net_ip_check( _net->filter, h->peer ) != 0 )
		{
			++(n->drops.count);
			continue;
		}

		++(n->accepts.count);

		// make sure we end in a newline or else we will trip over
		// ourself on daft apps that just send one line without a \n
		// or join on \n but don't append a trailing one
		if( b->buf[b->len-1] != '\n' )
			b->buf[b->len++]  = '\n';

		// and make sure to cap it all
		buf_terminate( b );

		// do a prefix check on that
		for( p = _net->prefix; p; p = p->next )
			if( iplist_test_ip( p->list, h->ip, &ipn ) != IPLIST_NOMATCH )
				break;

		if( ipn && ipn->tlen )
			(*(h->receiver))( udp_get_phost( n, ipn ), ib );
		else
			(*(h->receiver))( h, ib );
	}

	loop_mark_done( "udp", 0, 0 );

	mem_free_host( &h );
}




void udp_loop_flat( THRD *t )
{
	struct sockaddr_in sa;
	struct sockaddr *sp;
	socklen_t sl;
	NET_PORT *n;
	int sz, rb;
	IOBUF *ib;
	HOST *h;
	BUF *b;

	n = (NET_PORT *) t->arg;

	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = n->ip;
	sa.sin_port = htons( n->port );

	h  = mem_new_host( &sa, NET_BUF_SZ );
	ib = h->net->in;
	b  = ib->bf;

	h->type   = n->type;
	// for now we don't do prefixing or tokens on UDP
	net_set_host_parser( h, 0, 0 );

	loop_mark_start( "udp" );

	// we need to make room for a newline and a null for safety
	sz = b->sz - 2;
	// and let's not do constant casting
	sp = (struct sockaddr *) h->peer;

	while( RUNNING( ) )
	{
		// get a packet, set the from
		sl = sizeof( struct sockaddr_in );
		if( ( rb = recvfrom( n->fd, b->buf, sz, 0, sp, &sl ) ) < 0 )
		{
			if( errno == EINTR || errno == EAGAIN )
				continue;

			++(n->errors.count);
			err( "Recvfrom error -- %s", Err );
			loop_end( "receive error on udp socket" );
			break;
		}
		else if( !rb )
			continue;

		b->len = rb;

		++(n->accepts.count);

		// make sure we end in a newline or else we will trip over
		// ourself on daft apps that just send one line without a \n
		// or join on \n but don't append a trailing one
		if( b->buf[b->len-1] != '\n' )
			b->buf[b->len++]  = '\n';

		// and make sure to cap it all
		buf_terminate( b );

		// and try to parse that log
		(*(h->receiver))( h, ib );
	}

	loop_mark_done( "udp", 0, 0 );

	mem_free_host( &h );
}



int udp_listen( unsigned short port, uint32_t ip )
{
	struct sockaddr_in sa;
	struct timeval tv;
	int s;

	if( ( s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0 )
	{
		err( "Unable to make udp listen socket -- %s", Err );
		return -1;
	}

	tv.tv_sec  = _net->rcv_tmout;
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


