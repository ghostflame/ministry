/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* tcp.h - defines TCP handling functions                                  *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef CARBON_COPY_TCP_H
#define CARBON_COPY_TCP_H

throw_fn tcp_connection;
throw_fn tcp_loop;

#define lock_ntype( _t )					pthread_mutex_lock(   &((_t)->lock) )
#define unlock_ntype( _t )					pthread_mutex_unlock( &((_t)->lock) )

void tcp_disconnect( int *sock, char *name );
int tcp_listen( unsigned short port, uint32_t ip, int backlog );

#endif
