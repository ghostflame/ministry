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

#define TCP_THRD_DADDER             30
#define TCP_THRD_DSTATS             60
#define TCP_THRD_DGAUGE             10
#define TCP_THRD_DCOMPAT            20

#define TCP_MAX_POLLS               128

struct tcp_conn
{
    TCPCN                   *   next;
    HOST                    *   host;
};


struct tcp_thread
{
    struct pollfd           *   polls;
    int64_t                     pmax;
    int64_t                     pmin;
    int64_t                     curr;

    HOST                    *   hosts;
    HOST                    *   waiting;

    NET_TYPE                *   type;

    pthread_mutex_lock          lock;
};


throw_fn tcp_handler;
throw_fn tcp_watcher;
throw_fn tcp_loop;

void tcp_disconnect( int *sock, char *name );
int tcp_listen( unsigned short port, uint32_t ip, int backlog );

#endif
