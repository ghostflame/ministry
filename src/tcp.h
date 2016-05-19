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

throw_fn tcp_connection;
throw_fn tcp_loop;

void tcp_disconnect( int *sock, char *name );
int tcp_listen( unsigned short port, uint32_t ip, int backlog );

#endif
