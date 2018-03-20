/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* net.h - network structures, defaults and declarations                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef CARBON_COPY_NETWORK_H
#define CARBON_COPY_NETWORK_H

#define DEFAULT_RR_PORT					3901
#define DEFAULT_COAL_PORT				3801
#define DEFAULT_CARBON_PORT				2003


#define POLL_EVENTS						(POLLIN|POLLPRI|POLLRDNORM|POLLRDBAND|POLLHUP)

#define RR_NETBUF_SZ					0x10000	// 64k

#define DEFAULT_NET_BACKLOG				32
#define NET_DEAD_CONN_TIMER				600		// 10 mins
#define NET_RCV_TMOUT					3		// in sec
#define NET_RECONN_MSEC					3000	// in msec
#define NET_IO_MSEC						500		// in msec
#define NET_FLUSH_MSEC					2000	// in msec

#define NTYPE_ENABLED					0x0001
#define NTYPE_TCP_ENABLED				0x0002
#define NTYPE_UDP_ENABLED				0x0004
#define NTYPE_UDP_CHECKS				0x0008

#define DEFAULT_TARGET_HOST				"127.0.0.1"
#define DEFAULT_TARGET_PORT				2003	// graphite

#define NET_IP_HASHSZ					2003

#define HPRFX_BUFSZ						0x2000	// 8k



struct host_buffers
{
	HBUFS				*	next;
	RELAY				*	rule;
	IOBUF				**	bufs;
	int						bcount;
};




struct host_data
{
	HOST				*	next;
	SOCK				*	net;
	NET_TYPE			*	type;
	line_fn				*	parser;
	HBUFS				*	hbufs;
	struct sockaddr_in	*	peer;

	uint64_t				lines;
	uint64_t				relayed;
	uint64_t				overlength;
	uint64_t				olwarn;

	int64_t					last;

	IPNET				*	ipn;		// may well be null
	char				*	workbuf;	// gets set to fixed size
	char				*	ltarget;
	int						plen;
	int						lmax;

	uint32_t				ip;
};



struct net_in_port
{
	int						fd;
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
	char				*	label;
	char				*	name;

	pthread_mutex_t			lock;
	int32_t					conns;

	uint16_t				flags;
	uint16_t				udp_count;
	uint32_t				udp_bind;
};




struct network_control
{
	NET_TYPE			*	relay;

	char				*	filter_list;
	char				*	prefix_list;

	IPLIST				*	filter;
	IPLIST				*	prefix;

	time_t					dead_time;
	unsigned int			rcv_tmout;
	int						flush_nsec;
	int						max_bufs;
	int						tcount;
};



// set up a host with a prefix
int net_set_host_prefix( HOST *h, IPNET *n );

// init/shutdown
void net_start_type( NET_TYPE *nt );
int net_start( void );
void net_stop( void );

// config
NET_CTL *net_config_defaults( void );
int net_config_line( AVP *av );

#endif
