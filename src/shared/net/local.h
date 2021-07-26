/**************************************************************************
* Copyright 2015 John Denholm                                             *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
*                                                                         *
*                                                                         *
* net/local.h - network structures, defaults and declarations             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_NETWORK_LOCAL_H
#define SHARED_NETWORK_LOCAL_H


#define HPRFX_BUFSZ						0x4000	// 16k

// used to hash (port<<32)+ip to give a nice
// spread of tcp slots even with a power-of-two
// number of threads
#define TCP_MODULO_PRIME				1316779
#define NET_DEAD_CONN_TIMER				600     // 10 mins
#define NET_RCV_TMOUT					3       // in sec
#define NET_RECONN_MSEC					3000    // in msec
#define NET_IO_MSEC						500     // in msec
#define NET_IP_HASHSZ					2003




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

#define lock_tokens( )					pthread_mutex_lock(   &(_net->tokens->lock) )
#define unlock_tokens( )				pthread_mutex_unlock( &(_net->tokens->lock) )



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


void tcp_close_active_host( HOST *h );
void tcp_close_host( HOST *h );
int net_set_host_prefix( HOST *h, IPNET *n );

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
