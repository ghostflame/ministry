/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* net/local.h - network structures, defaults and declarations             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_NETWORK_LOCAL_H
#define SHARED_NETWORK_LOCAL_H


#define TCP_MAX_POLLS					128
// used to hash (port<<32)+ip to give a nice
// spread of tcp slots even with a power-of-two
// number of threads
#define TCP_MODULO_PRIME				1316779
#define DEFAULT_NET_BACKLOG				32
#define NET_DEAD_CONN_TIMER				600     // 10 mins
#define NET_RCV_TMOUT					3       // in sec
#define NET_RECONN_MSEC					3000    // in msec
#define NET_IO_MSEC						500     // in msec

#define NTYPE_ENABLED					0x0001
#define NTYPE_TCP_ENABLED				0x0002
#define NTYPE_UDP_ENABLED				0x0004
#define NTYPE_UDP_CHECKS				0x0008


enum tcp_style_types
{
	TCP_STYLE_POOL = 0,
	TCP_STYLE_THRD,
	TCP_STYLE_EPOLL,
	TCP_STYLE_MAX
};


#include "shared.h"

#define POLL_EVENTS						(POLLIN|POLLPRI|POLLRDNORM|POLLRDBAND|POLLHUP)
#define EPOLL_EVENTS					(EPOLLIN|EPOLLPRI|EPOLLRDHUP|EPOLLET)


#define terr( fmt, ... )				err( "[TCP:%03d] " fmt, th->num, ##__VA_ARGS__ )
#define twarn( fmt, ... )				warn( "[TCP:%03d] " fmt, th->num, ##__VA_ARGS__ )
#define tnotice( fmt, ... )				notice( "[TCP:%03d] " fmt, th->num, ##__VA_ARGS__ )
#define tinfo( fmt, ... )				info( "[TCP:%03d] " fmt, th->num, ##__VA_ARGS__ )
#define tdebug( fmt, ... )				debug( "[TCP:%03d] " fmt, th->num, ##__VA_ARGS__ )


#define lock_ntype( nt )				pthread_mutex_lock(   &(nt->lock) )
#define unlock_ntype( nt )				pthread_mutex_unlock( &(nt->lock) )

#define lock_tcp( th )					pthread_mutex_lock(   &(th->lock) )
#define unlock_tcp( th )				pthread_mutex_unlock( &(th->lock) )


struct tcp_style_data
{
	char				*	name;
	int						style;
	tcp_setup_fn		*	setup;
	tcp_fn				*	hdlr;
};


struct tcp_thread
{
	NET_PORT			*	port;
	NET_TYPE			*	type;

	struct pollfd		*	polls;
	int64_t					pmax;   // highest in-use poll slot
	int64_t					pmin;   // lowest free poll slot
	int64_t					curr;   // how many current connections

	int						ep_fd;  // used for epoll
	struct epoll_event	*	ep_events;

	HOST				**	hosts;  // used by pool
	HOST				*	hlist;  // used by epoll
	HOST				*	waiting;

	pthread_mutex_t			lock;
	pthread_t				tid;
	int64_t					num;
};

extern const struct tcp_style_data tcp_styles[];
extern uint32_t net_masks[33];

void tcp_close_active_host( HOST *h );
void tcp_close_host( HOST *h );

tcp_fn tcp_choose_thread;
tcp_fn tcp_throw_thread;

tcp_setup_fn tcp_epoll_setup;
tcp_setup_fn tcp_pool_setup;
tcp_setup_fn tcp_thrd_setup;

throw_fn tcp_pool_thread;
throw_fn tcp_epoll_thread;
throw_fn tcp_thrd_thread;

// and our global
extern NET_CTL *_net;

#endif
