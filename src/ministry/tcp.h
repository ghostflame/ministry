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

struct tcp_style_data
{
	char				*	name;
	int						style;
	tcp_setup_fn		*	setup;
	tcp_fn				*	fp;
};

enum tcp_style_types
{
	TCP_STYLE_POOL = 0,
	TCP_STYLE_THRD,
	TCP_STYPE_EPOLL,
	TCP_STYLE_MAX
};

extern const struct tcp_style_data tcp_styles[];


throw_fn tcp_loop;

void tcp_close_host( HOST *h );
void tcp_close_active_host( HOST *h );
HOST *tcp_get_host( int sock, NET_PORT *np );
int tcp_listen( unsigned short port, uint32_t ip, int backlog );

#endif
