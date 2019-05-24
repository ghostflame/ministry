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

http_callback http_calls_metrics;
http_callback http_calls_stats;
http_callback http_calls_count;
http_callback http_calls_usage;

int http_calls_ctl_iterator( void *cls, enum MHD_ValueKind kind, const char *key, const char *filename,
        const char *content_type, const char *transfer_encoding, const char *data, uint64_t off, size_t size );

http_callback http_calls_ctl_init;
http_callback http_calls_ctl_done;

void http_calls_init( void );

#endif

