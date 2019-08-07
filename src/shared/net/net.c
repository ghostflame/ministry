/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* net.c - handles network functions                                       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"

uint32_t net_masks[33] = {
	0x00000000,
	0x00000001,
	0x00000003,
	0x00000007,
	0x0000000f,
	0x0000001f,
	0x0000003f,
	0x0000007f,
	0x000000ff,
	0x000001ff,
	0x000003ff,
	0x000007ff,
	0x00000fff,
	0x00001fff,
	0x00003fff,
	0x00007fff,
	0x0000ffff,
	0x0001ffff,
	0x0003ffff,
	0x0007ffff,
	0x000fffff,
	0x001fffff,
	0x003fffff,
	0x007fffff,
	0x00ffffff,
	0x01ffffff,
	0x03ffffff,
	0x07ffffff,
	0x0fffffff,
	0x1fffffff,
	0x3fffffff,
	0x7fffffff,
	0xffffffff
};



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


