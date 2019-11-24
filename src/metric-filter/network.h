/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* network.h - defines network functions and defaults                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef METRIC_FILTER_NETWORK_H
#define METRIC_FILTER_NETWORK_H

#define DEFAULT_METFILT_PORT		2030


struct network_control
{
	NET_TYPE			*	port;
};


NETW_CTL *network_config_defaults( void );


#endif
