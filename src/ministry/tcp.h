/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* tcp.h - defines TCP handling functions                                  *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_TCP_H
#define MINISTRY_TCP_H

enum tcp_style_types
{
	TCP_STYLE_POOL = 0,
	TCP_STYLE_THRD,
	TCP_STYLE_EPOLL,
	TCP_STYLE_MAX
};


#define TCP_THRD_DADDER             30
#define TCP_THRD_DSTATS             60
#define TCP_THRD_DGAUGE             10
#define TCP_THRD_DCOMPAT            20

#define TCP_MAX_POLLS               128

// used to hash (port<<32)+ip to give a nice
// spread of tcp slots even with a power-of-two
// number of threads
#define TCP_MODULO_PRIME			1316779

#define POLL_EVENTS						(POLLIN|POLLPRI|POLLRDNORM|POLLRDBAND|POLLHUP)
#define EPOLL_EVENTS					(EPOLLIN|EPOLLPRI|EPOLLRDHUP|EPOLLET)

#define terr( fmt, ... )        err( "[TCP:%03d] " fmt, th->num, ##__VA_ARGS__ )
#define twarn( fmt, ... )       warn( "[TCP:%03d] " fmt, th->num, ##__VA_ARGS__ )
#define tnotice( fmt, ... )     notice( "[TCP:%03d] " fmt, th->num, ##__VA_ARGS__ )
#define tinfo( fmt, ... )       info( "[TCP:%03d] " fmt, th->num, ##__VA_ARGS__ )
#define tdebug( fmt, ... )      debug( "[TCP:%03d] " fmt, th->num, ##__VA_ARGS__ )


struct tcp_thread
{
	struct pollfd		*	polls;

	int64_t					pmax;	// highest in-use poll slot
	int64_t					pmin;	// lowest free poll slot
	int64_t					curr;	// how many current connections

	HOST				**	hosts;	// used by pool
	HOST				*	hlist;	// used by epoll
	HOST				*	waiting;

	NET_PORT			*	port;
	NET_TYPE			*	type;

	pthread_mutex_t			lock;
	pthread_t				tid;
	int64_t					num;

	int						ep_fd;	// used for epoll
	struct epoll_event	*	ep_events;
};


struct tcp_style_data
{
	char				*	name;
	int						style;
	tcp_setup_fn		*	setup;
	tcp_fn				*	hdlr;
};

extern const struct tcp_style_data tcp_styles[];

tcp_fn tcp_choose_thread;
tcp_fn tcp_throw_thread;

tcp_setup_fn tcp_epoll_setup;
tcp_setup_fn tcp_pool_setup;
tcp_setup_fn tcp_thrd_setup;

throw_fn tcp_pool_thread;
throw_fn tcp_epoll_thread;
throw_fn tcp_thrd_thread;

throw_fn tcp_loop;

void tcp_close_host( HOST *h );
void tcp_close_active_host( HOST *h );
HOST *tcp_get_host( int sock, NET_PORT *np );
int tcp_listen( unsigned short port, uint32_t ip, int backlog );

#endif
