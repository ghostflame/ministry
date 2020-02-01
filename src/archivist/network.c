/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* net.c - networking setup and config                                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "archivist.h"



const char *network_type_id_names[NETW_TYPE_MAX] =
{
	"line",
	"binary",
	"lineTLS",
	"binaryTLS"
};

const char *network_data_type_id_names[NETW_DATA_TYPE_MAX] =
{
	"line",
	"binary"
};


NWTYPE network_type_defns[NETW_TYPE_MAX] =
{
	{
		.type		= NETW_TYPE_LINE_PLAIN,
		.dtype		= NETW_DATA_TYPE_LINE,
		.port		= DEFAULT_LINE_PORT,
		.tls		= 0,
		.style		= TCP_STYLE_THRD,
		.buffp		= data_parse_line,
		.cert_path	= NULL,
		.key_path	= NULL,
		.cert		= NULL,
		.key		= NULL,
		.key_pass	= NULL,
		.nt			= NULL
	},
	{
		.type		= NETW_TYPE_BIN_PLAIN,
		.dtype		= NETW_DATA_TYPE_BIN,
		.port		= DEFAULT_BIN_PORT,
		.tls		= 0,
		.style		= TCP_STYLE_EPOLL,
		.buffp		= data_parse_bin,
		.cert_path	= NULL,
		.key_path	= NULL,
		.cert		= NULL,
		.key		= NULL,
		.key_pass	= NULL,
		.nt			= NULL
	},
	{
		.type		= NETW_TYPE_LINE_TLS,
		.dtype		= NETW_DATA_TYPE_LINE,
		.port		= DEFAULT_LINE_TLS_PORT,
		.tls		= 1,
		.style		= TCP_STYLE_THRD,
		.buffp		= data_parse_line,
		.cert_path	= NULL,
		.key_path	= NULL,
		.cert		= NULL,
		.key		= NULL,
		.key_pass	= NULL,
		.nt			= NULL
	},
	{
		.type		= NETW_TYPE_BIN_TLS,
		.dtype		= NETW_DATA_TYPE_BIN,
		.port		= DEFAULT_BIN_TLS_PORT,
		.tls		= 1,
		.style		= TCP_STYLE_THRD,
		.buffp		= data_parse_bin,
		.cert_path	= NULL,
		.key_path	= NULL,
		.cert		= NULL,
		.key		= NULL,
		.key_pass	= NULL,
		.nt			= NULL
	}
};



NET_TYPE *network_type_defaults( int8_t type )
{
	NWTYPE *n = network_type_defns + type;
	NET_TYPE *nt;

	nt              = (NET_TYPE *) allocz( sizeof( NET_TYPE ) );
	nt->tcp         = (NET_PORT *) allocz( sizeof( NET_PORT ) );
	nt->tcp->ip     = INADDR_ANY;
	nt->tcp->back   = DEFAULT_NET_BACKLOG;
	nt->tcp->port   = n->port;
	nt->tcp->type   = nt;
	nt->buf_parser  = n->buffp;
	nt->udp_bind    = INADDR_ANY;
	nt->pollmax     = TCP_MAX_POLLS;
	nt->tcp_style   = n->style;
	nt->name        = strdup( network_type_id_names[type] );
	nt->flags       = NTYPE_ENABLED|NTYPE_TCP_ENABLED|NTYPE_UDP_ENABLED;

	// and hook up the dtype
	n->nt = nt;

	net_add_type( nt );
	return nt;
}



NETW_CTL *network_config_defaults( void )
{
	NETW_CTL *netw;

	netw         = (NETW_CTL *) allocz( sizeof( NETW_CTL ) );
	netw->line   = network_type_defaults( NETW_TYPE_LINE_PLAIN );
	netw->bin    = network_type_defaults( NETW_TYPE_BIN_PLAIN );

	return netw;
}




