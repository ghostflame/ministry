/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* net.h - network structures, defaults and declarations                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef CARBON_COPY_NETWORK_H
#define CARBON_COPY_NETWORK_H

#define DEFAULT_RR_PORT					3901
#define DEFAULT_COAL_PORT				3801
#define DEFAULT_CARBON_PORT				2003


#define NET_IO_MSEC						500		// in msec
#define NET_FLUSH_MSEC					2000	// in msec

#define DEFAULT_TARGET_HOST				"127.0.0.1"
#define DEFAULT_TARGET_PORT				2003	// graphite


struct host_buffers
{
	HBUFS				*	next;
	RELAY				*	rule;
	IOBUF				**	bufs;
	int						bcount;
};



struct network_control
{
	NET_TYPE			*	relay;
};



// config
NETW_CTL *network_config_defaults( void );

#endif
