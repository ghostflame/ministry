/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* net.c - networking setup and config                                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "carbon_copy.h"



NETW_CTL *network_config_defaults( void )
{
	NETW_CTL *net;
	NET_TYPE *nt;

	net              = (NETW_CTL *) allocz( sizeof( NETW_CTL ) );
	net->flush_nsec  = 1000000 * NET_FLUSH_MSEC;

	nt               = (NET_TYPE *) allocz( sizeof( NET_TYPE ) );
	nt->tcp          = (NET_PORT *) allocz( sizeof( NET_PORT ) );
	nt->tcp->ip      = INADDR_ANY;
	nt->tcp->back    = DEFAULT_NET_BACKLOG;
	nt->tcp->port    = DEFAULT_RR_PORT;
	nt->tcp->type    = nt;
	nt->flat_parser  = &relay_simple;
	nt->prfx_parser  = &relay_prefix;
	nt->buf_parser   = &relay_parse_buf;
	nt->udp_bind     = INADDR_ANY;
	nt->label        = strdup( "relay" );
	nt->name         = strdup( "relay" );
	nt->flags        = NTYPE_ENABLED|NTYPE_TCP_ENABLED|NTYPE_UDP_ENABLED;

	net_add_type( nt );

	net->relay       = nt;

	// can't add default target, it's a linked list

	return net;
}




int network_config_line( AVP *av )
{
	if( attIs( "flush_msec" ) )
	{
		ctl->net->flush_nsec = 1000000 * atoll( av->vptr );
		if( ctl->net->flush_nsec < 0 )
			ctl->net->flush_nsec = 1000000 * NET_FLUSH_MSEC;
		debug( "Host flush time set to %ld usec.", ctl->net->flush_nsec / 1000 );
	}
	else
		return -1;

	return 0;
}


