/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* net.h - network structures, defaults and declarations                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_NETWORK_H
#define MINISTRY_NETWORK_H

#define DEFAULT_STATS_PORT				9125
#define DEFAULT_ADDER_PORT				9225
#define DEFAULT_GAUGE_PORT				9325
#define DEFAULT_COMPAT_PORT				8125

#define MIN_NETBUF_SZ					0x10000	// 64k

#define DEFAULT_NET_BACKLOG				32
#define NET_DEAD_CONN_TIMER				600		// 10 mins
#define NET_RCV_TMOUT					3		// in sec
#define NET_RECONN_MSEC					3000	// in msec
#define NET_IO_MSEC						500		// in msec

#define NTYPE_ENABLED					0x0001
#define NTYPE_TCP_ENABLED				0x0002
#define NTYPE_UDP_ENABLED				0x0004
#define NTYPE_UDP_CHECKS				0x0008

#define DEFAULT_TARGET_HOST				"127.0.0.1"
#define DEFAULT_TARGET_PORT				2003	// graphite

#define NET_IP_HASHSZ					2003

#define HPRFX_BUFSZ						0x2000	// 8k



struct network_control
{
	NET_TYPE			*	stats;
	NET_TYPE			*	adder;
	NET_TYPE			*	gauge;
	NET_TYPE			*	compat;

	TOKENS				*	tokens;

	IPLIST				*	filter;
	NET_PFX				*	prefix;

	char				*	filter_list;

	time_t					dead_time;
	unsigned int			rcv_tmout;
	int64_t					tcount;
};



// set up a host with a prefix
int net_set_host_prefix( HOST *h, IPNET *n );

// set a host parser fn
int net_set_host_parser( HOST *h, int token_check, int prefix_check );

void net_disconnect( int *sock, char *name );

// dns
int net_lookup_host( char *host, struct sockaddr_in *res );

// init/shutdown
void net_start_type( NET_TYPE *nt );
int net_start( void );
void net_stop( void );

// config
NET_CTL *net_config_defaults( void );
int net_config_line( AVP *av );

#endif
