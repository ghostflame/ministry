/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* net.h - network structures, defaults and declarations                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_NETWORK_H
#define MINISTRY_NETWORK_H

#define DEFAULT_STATS_PORT				9125
#define DEFAULT_ADDER_PORT				9225
#define DEFAULT_GAUGE_PORT				9325
#define DEFAULT_HISTO_PORT				9425
#define DEFAULT_COMPAT_PORT				8125

#define DEFAULT_TARGET_HOST				"127.0.0.1"
#define DEFAULT_TARGET_PORT				2003	// graphite



struct network_control
{
	NET_TYPE			*	stats;
	NET_TYPE			*	adder;
	NET_TYPE			*	gauge;
	NET_TYPE			*	histo;
	NET_TYPE			*	compat;
};



// config
NETW_CTL *network_config_defaults( void );


#endif
