/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* udp.h - defines UDP handling functions                                  *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_UDP_H
#define MINISTRY_UDP_H

throw_fn udp_loop_flat;
throw_fn udp_loop_checks;

int udp_listen( unsigned short port, uint32_t ip );

iplist_data_fn udp_add_phost;

#endif
