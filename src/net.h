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

#define IP_NET_WHITELIST				0
#define IP_NET_BLACKLIST				1

#define POLL_EVENTS						(POLLIN|POLLPRI|POLLRDNORM|POLLRDBAND|POLLHUP)

#define MIN_NETBUF_SZ					0x10000	// 64k

#define HOST_CLOSE						0x01
#define HOST_CLOSE_EMPTY				0x02

#define DEFAULT_NET_BACKLOG				32
#define NET_DEAD_CONN_TIMER				600		// 10 mins
#define NET_RCV_TMOUT					3		// in sec
#define NET_RECONN_MSEC					3000	// in msec
#define NET_IO_MSEC						500		// in msec

#define NTYPE_ENABLED					0x0001
#define NTYPE_TCP_ENABLED				0x0002
#define NTYPE_UDP_ENABLED				0x0004

#define DEFAULT_TARGET_HOST				"127.0.0.1"
#define DEFAULT_TARGET_PORT				2003	// graphite



struct ip_network
{
	IPNET				*	next;
	uint32_t				net;
	uint16_t				bits;
	uint16_t				type;
};


struct ip_check
{
	IPNET				*	list;
	int						deflt;
	int						verbose;
	int						enabled;
};



// socket for talking to a host
struct net_socket
{
	IOBUF				*	out;
	IOBUF				*	in;

	int						sock;
	int						flags;
	int						bufs;

	struct sockaddr_in		peer;
	char				*	name;
};



struct host_data
{
	HOST				*	next;
	NSOCK				*	net;

	WORDS				*	val;		// per line

	NET_TYPE			*	type;

	struct sockaddr_in		peer;

	uint64_t				points;
	uint64_t				invalid;
};



struct net_in_port
{
	int						sock;
	uint64_t				counter;

	uint16_t				port;
	uint16_t				back;
	uint32_t				ip;

	NET_TYPE			*	type;
};


struct net_type
{
	NET_PORT			*	tcp;
	NET_PORT			**	udp;
	line_fn				*	parser;
	add_fn				*	handler;
	char				*	label;

	uint16_t				flags;
	uint16_t				udp_count;
	uint32_t				udp_bind;
};


struct network_target
{
	TARGET				*	next;
	NSOCK				*	sock;
	IOLIST				*	qhead;
	IOLIST				*	qend;
	char				*	host;

	uint32_t				reconn_ct;
	uint32_t				countdown;
	uint16_t				port;
	int16_t					bufs;

	// offsets into the current buffer
	int						curr_off;
	int						curr_len;

	pthread_mutex_t			lock;
};



struct network_control
{
	NET_TYPE			*	stats;
	NET_TYPE			*	adder;
	NET_TYPE			*	gauge;
	NET_TYPE			*	compat;

	TARGET				*	targets;

	IPCHK				*	ipcheck;

	time_t					dead_time;
	unsigned int			rcv_tmout;
	int						reconn;
	int						io_usec;
	int						max_bufs;
	int						tcount;
};



// thread control
throw_fn net_watched_socket;

// client connections
HOST *net_get_host( int sock, NET_TYPE *type );
void net_close_host( HOST *h );

NSOCK *net_make_sock( int insz, int outsz, struct sockaddr_in *peer );
//int net_port_sock( PORT_CTL *pc, uint32_t ip, int backlog );
void net_disconnect( int *sock, char *name );


// init/shutdown
int net_start( void );
void net_stop( void );

// config
NET_CTL *net_config_defaults( void );
int net_config_line( AVP *av );

#endif
