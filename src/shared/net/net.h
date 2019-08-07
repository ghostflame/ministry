/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* net.h - network structures, defaults and declarations                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_NETWORK_H
#define SHARED_NETWORK_H


#define MIN_NETBUF_SZ						0x10000		// 64k


struct net_prefix
{
	NET_PFX             *   next;
	IPLIST              *   list;
	char                *   name;
	char                *   text;
	int                     tlen;
};




struct net_in_port
{
	NET_TYPE			*	type;

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

	TCPTH				**	threads;
};



struct net_type
{
	NET_TYPE			*	next;	// used in tokening

	NET_PORT			*	tcp;
	NET_PORT			**	udp;

	uint16_t				flags;
	uint16_t				udp_count;
	uint32_t				udp_bind;

	buf_fn				*	buf_parser;
	line_fn				*	flat_parser;
	line_fn				*	prfx_parser;
	add_fn				*	handler;

	char				*	label;
	char				*	name;

	pthread_mutex_t			lock;
	int64_t					conns;
	int64_t					threads;
	int64_t					pollmax;

	tcp_setup_fn		*	tcp_setup;
	tcp_fn				*	tcp_hdlr;
	int						tcp_style;

	int						token_bit;
};



struct host_data
{
	HOST				*	next;
	SOCK				*	net;

	NET_TYPE			*	type;
	NET_PORT			*	port;

	line_fn				*	parser;
	add_fn				*	handler;
	buf_fn				*	receiver;

	struct sockaddr_in	*	peer;

	void				*	data;

	uint64_t				lines;
	uint64_t				handled;
	uint64_t				invalid;

	uint64_t				overlength;
	uint64_t				olwarn;

	// connection orienting
	int64_t					connected;	// first timestamp seen (nsec)
	int64_t					last;		// last timestamp seen (nsec)

	struct epoll_event		ep_evt;		// used in epoll

	// filtering and rules
	IPNET				*	ipn;		// may well be null
	char				*	workbuf;	// gets set to fixed size
	char				*	ltarget;
	int						plen;
	int						lmax;
	int						quiet;

	uint32_t				ip;			// easier than always hitting the peer
};



struct net_control
{
	char				*	filter_list;
	IPLIST				*	filter;

	NET_TYPE			*	ntypes;

	int64_t					dead_time;
	unsigned int			rcv_tmout;
};



throw_fn tcp_loop;

int tcp_listen( unsigned short port, uint32_t ip, int backlog );

int net_lookup_host( char *str, struct sockaddr_in *res );
int net_ip_check( IPLIST *l, struct sockaddr_in *sin );

#endif
