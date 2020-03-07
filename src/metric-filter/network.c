/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* network.c - networking setup and config                                 *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "metric_filter.h"



NETW_CTL *network_config_defaults( void )
{
	NETW_CTL *net;
	NET_TYPE *nt;

	nt               = (NET_TYPE *) mem_perm( sizeof( NET_TYPE ) );
	nt->tcp          = (NET_PORT *) mem_perm( sizeof( NET_PORT ) );
	nt->tcp->ip      = INADDR_ANY;
	nt->tcp->back    = DEFAULT_NET_BACKLOG;
	nt->tcp->port    = DEFAULT_METFILT_PORT;
	nt->tcp->type    = nt;
	nt->tcp_style    = TCP_STYLE_THRD;
	nt->buf_parser   = &filter_parse_buf;
	nt->udp_bind     = INADDR_ANY;
	nt->label        = str_perm( "filter", 6 );
	nt->name         = str_perm( "filter", 6 );
	nt->flags        = NTYPE_ENABLED|NTYPE_TCP_ENABLED|NTYPE_UDP_ENABLED;

	net_add_type( nt );

	net              = (NETW_CTL *) mem_perm( sizeof( NETW_CTL ) );
	net->port        = nt;

	// can't add default target, it's a linked list

	return net;
}


