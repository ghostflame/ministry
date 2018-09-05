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



struct host_data
{
	HOST				*	next;
	SOCK				*	net;

	NET_TYPE			*	type;
	NET_PORT			*	port;
	line_fn				*	parser;
	add_fn				*	handler;

	struct sockaddr_in	*	peer;

	uint64_t				points;
	uint64_t				invalid;

	int64_t					connected;	// first timestamp seen (nsec)
	time_t					last;		// last timestamp sec

	struct epoll_event		ep_evt;		// used in epoll option

	IPNET				*	ipn;
	char				*	workbuf;	// gets set to fixed size
	char				*	ltarget;
	int						plen;
	int						lmax;
	int						quiet;

	uint32_t				ip;			// because we want it
};



struct net_in_port
{
	int						fd;
	uint64_t				counter;

	uint16_t				port;
	uint16_t				back;
	uint32_t				ip;

	TCPTH				**	threads;

	LLCT					errors;
	LLCT					drops;
	LLCT					accepts;

	HOST				**	phosts;
	uint64_t				phsz;

	NET_TYPE			*	type;
};


struct net_type
{
	NET_PORT			*	tcp;
	NET_PORT			**	udp;
	line_fn				*	flat_parser;
	line_fn				*	prfx_parser;
	add_fn				*	handler;
	char				*	label;
	char				*	name;

	pthread_mutex_t			lock;
	int64_t					threads;
	int64_t					pollmax;
	int64_t					conns;

	uint16_t				flags;
	uint16_t				udp_count;
	uint32_t				udp_bind;

	int32_t					token_type;

	int						tcp_style;
	tcp_setup_fn		*	tcp_setup;
	tcp_fn				*	tcp_hdlr;
};






struct network_control
{
	NET_TYPE			*	stats;
	NET_TYPE			*	adder;
	NET_TYPE			*	gauge;
	NET_TYPE			*	compat;

	TOKENS				*	tokens;

	IPLIST				*	filter;
	IPLIST				*	prefix;

	char				*	filter_list;
	char				*	prefix_list;

	time_t					dead_time;
	unsigned int			rcv_tmout;
	int64_t					tcount;
};



// set up a host with a prefix
int net_set_host_prefix( HOST *h, IPNET *n );

// set a host parser fn
int net_set_host_parser( HOST *h, int token_check, int prefix_check );

//int net_port_sock( PORT_CTL *pc, uint32_t ip, int backlog );
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
