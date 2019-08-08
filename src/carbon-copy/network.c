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
	NET_TYPE *nt;

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

	net              = (NETW_CTL *) allocz( sizeof( NETW_CTL ) );
	net->relay       = nt;

	// can't add default target, it's a linked list

	return net;
}


