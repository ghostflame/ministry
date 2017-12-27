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

extern uint32_t net_masks[33];

int net_lookup_host( char *str, struct sockaddr_in *res );
int net_ip_check( IPLIST *l, struct sockaddr_in *sin );




#endif
