/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* net.c - networking setup and config                                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry_test.h"


// these need init'd, done in net_config_defaults
uint32_t net_masks[33];


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
		ns->in = mem_new_buf( insz, 0 );

	if( outsz )
		ns->out = mem_new_buf( outsz, 0 );

	// no socket yet
	ns->sock = -1;

	return ns;
}



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


