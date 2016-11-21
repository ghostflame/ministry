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
#define DEFAULT_HTTP_PORT				9080
#define DEFAULT_HTTPS_PORT				9443

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
#define NTYPE_UDP_CHECKS				0x0008

#define DEFAULT_TARGET_HOST				"127.0.0.1"
#define DEFAULT_TARGET_PORT				2003	// graphite


// ip address regex
#define NET_IP_REGEX_OCT				"(25[0-5]|2[0-4][0-9]||1[0-9][0-9]|[1-9][0-9]|[0-9])"
#define NET_IP_REGEX_STR				"^((" NET_IP_REGEX_OCT "\\.){3}" NET_IP_REGEX_OCT ")(/(3[0-2]|[12]?[0-9]))?$"

#define NET_IP_HASHSZ					2003

#define NET_IP_WHITELIST				0
#define NET_IP_BLACKLIST				1

#define HPRFX_BUFSZ						0x2000	// 8k


struct host_prefix
{
	HPRFX				*	next;
	char				*	pstr;
	char				*	confstr;
	int32_t					plen;
};


struct ip_network
{
	IPNET				*	next;
	HPRFX				*	prefix;
	uint32_t				ipnet;
	uint16_t				bits;
	uint16_t				act;
};



struct ip_check
{
	IPNET				**	ips;
	IPNET				*	nets;

	char				*	name;

	int32_t					total;
	int32_t					hashsz;

	uint32_t				enable : 1;
	uint32_t				verbose : 1;
	uint32_t				drop : 1;
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

	NET_TYPE			*	type;
	line_fn				*	parser;

	struct sockaddr_in	*	peer;

	uint64_t				points;
	uint64_t				invalid;

	HPRFX				*	prefix;		// may well be null
	char				*	workbuf;	// gets set to fixed size
	char				*	ltarget;
	int						plen;
	int						lmax;
};



struct net_in_port
{
	int						sock;
	uint64_t				counter;

	uint16_t				port;
	uint16_t				back;
	uint32_t				ip;

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
	int32_t					conns;

	uint16_t				flags;
	uint16_t				udp_count;
	uint32_t				udp_bind;

	int32_t					token_type;
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

	TOKENS				*	tokens;

	IPCHK				*	iplist;
	IPCHK				*	prefix;
	regex_t					ipregex;

	time_t					dead_time;
	unsigned int			rcv_tmout;
	int						reconn;
	int						io_usec;
	int						max_bufs;
	int						tcount;
};



// do we have a prefix?
HPRFX *net_prefix_check( struct sockaddr_in *sin );
int net_ip_check( struct sockaddr_in *sin );

// set up a host with a prefix
int net_set_host_prefix( HOST *h, HPRFX *pr );

// set a host parser fn
int net_set_host_parser( HOST *h, int token_check, int prefix_check );

NSOCK *net_make_sock( int insz, int outsz, struct sockaddr_in *peer );
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
