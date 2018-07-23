/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* tcp.h - defines TCP handling functions                                  *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_TCP_EPOLL_H
#define MINISTRY_TCP_EPOLL_H

#define EPOLL_EVENTS		(EPOLLIN|EPOLLPRI|EPOLLRDHUP|EPOLLET)

tcp_setup_fn tcp_epoll_setup;
tcp_fn tcp_epoll_handler;

#endif
