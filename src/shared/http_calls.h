/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* http_calls.h - built-in http endpoint definitions                       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_HTTP_CALLS_H
#define SHARED_HTTP_CALLS_H

http_callback http_calls_stats;
http_callback http_calls_usage;

void http_calls_init( void );

#endif

