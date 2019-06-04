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

int tcp_listen( unsigned short port, uint32_t ip, int backlog );

#endif
