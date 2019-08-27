/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* relay.h - defines relay structures and functions                        * 
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#ifndef CARBON_COPY_RELAY_LOCAL_H
#define CARBON_COPY_RELAY_LOCAL_H


#define LINE_SEPARATOR				'\n'
#define RELAY_MAX_REGEXES			32

#define RELAY_FLUSH_MSEC			1000L	// in msec
#define RELAY_FLUSH_MSEC_FAST		25L		// for real-time stats
#define RELAY_FLUSH_MSEC_SLOW		5000L	// for data with timestamps in - no rush


#include "../carbon_copy.h"




struct relay_line
{
	char				*	line;
	int32_t					len;
	int16_t					plen;
	char					sep;
};



extern RLY_CTL *_relay;




#endif

