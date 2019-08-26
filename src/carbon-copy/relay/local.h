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

#define lock_rdata( _r )			pthread_mutex_lock(   &(_r->lock) )
#define unlock_rdata( _r )			pthread_mutex_unlock( &(_r->lock) )

#include "../carbon_copy.h"

enum relay_rule_types
{
	RTYPE_UNKNOWN = 0,
	RTYPE_REGEX,
	RTYPE_HASH,
	RTYPE_MAX
};

#define RTYPE_REGEX					1
#define RTYPE_HASH					2


struct relay_line
{
	char				*	line;
	int32_t					len;
	int16_t					plen;
	char					sep;
};



extern RLY_CTL *_relay;




#endif

