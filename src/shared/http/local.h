/**************************************************************************
* Copyright 2015 John Denholm                                             *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
*                                                                         *
*                                                                         *
* http/local.h - http structure and fn definitions                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_HTTP_LOCAL_H
#define SHARED_HTTP_LOCAL_H


#ifndef MIN_MHD_PASS
  #if MHD_VERSION > 0x00094000
    #define MIN_MHD_PASS 1
  #else
    #define MIN_MHD_PASS 0
  #endif
#endif


#define DEFAULT_HTTP_PORT               9080
#define DEFAULT_HTTPS_PORT              9443

#define DEFAULT_HTTP_CONN_LIMIT         256
#define DEFAULT_HTTP_CONN_IP_LIMIT      64
#define DEFAULT_HTTP_CONN_TMOUT         10

#define DEFAULT_POST_BUF_SZ             0x4000		// 16k
#define DEFAULT_POST_MAX_SZ             0x200000	// 2M

#define MAX_TLS_FILE_SIZE               0x10000		// 64k
#define MAX_TLS_PASS_SIZE               512


#include "shared.h"


typedef void * (*cb_RequestLogger) ( void *cls, const char *uri, HTTP_CONN *conn );




struct http_tls_file
{
    const char              *   type;
    char                    *   path;
    const unsigned char     *   content;
    int                         len;
};


struct http_tls
{
    TLS_FILE                    key;
    TLS_FILE                    cert;
    volatile char           *   password;
    char                    *   priorities;
    int                         enabled;

    uint16_t                    port;
    uint16_t                    passlen;
};


struct http_callbacks
{
    MHD_PanicCallback                       panic;
    MHD_AcceptPolicyCallback                policy;
    MHD_AccessHandlerCallback               handler;
    MHD_RequestCompletedCallback            complete;
    MHD_ContentReaderCallback               reader;
    MHD_ContentReaderFreeCallback           rfree;

    MHD_LogCallback                         log;
    cb_RequestLogger                        reqlog;
};

struct http_handlers
{
    HTPATH              *   list;
    char                *   method;
    int                     count;
};


#define lock_hits( )        pthread_mutex_lock(   &(_http->hitlock) )
#define unlock_hits( )      pthread_mutex_unlock( &(_http->hitlock) )

#define hit_counter( _p )   lock_hits( ); (_p)++; unlock_hits( )


// unused fns
ssize_t http_unused_reader( void *cls, uint64_t pos, char *buf, size_t max );
void http_unused_reader_free( void *cls );

// request handling
enum MHD_Result http_get_url_param( void *cls, HTTP_VAL kind, const char *key, const char *value );
int http_parse_url( HTREQ *req, char *url );
enum MHD_Result http_access_policy( void *cls, const struct sockaddr *addr, socklen_t addrlen );
enum MHD_Result http_send_response( HTREQ *req );
void http_request_complete( void *cls, HTTP_CONN *conn, void **arg, HTTP_CODE toe );
int http_request_access_check( IPLIST *src, HTREQ *req, struct sockaddr *sa );
HTREQ *http_request_creator( HTTP_CONN *conn, const char *url, const char *method );

enum MHD_Result http_request_handler( void *cls, HTTP_CONN *conn, const char *url,
	const char *method, const char *version, const char *upload_data,
	size_t *upload_data_size, void **con_cls );


// utils
void http_server_panic( void *cls, const char *file, unsigned int line, const char *reason );
void *http_log_request( void *cls, const char *uri, HTTP_CONN *conn );
void http_log( void *arg, const char *fm, va_list ap );
void http_set_reporter( http_reporter *fp, void *arg );

sort_fn __http_cmp_handlers;


// init
void http_clean_password( HTTP_CTL *h );


// handlers
HTPATH *http_find_callback( const char *url, int rlen, HTHDLS *hd );


// calls
http_callback http_calls_metrics;
http_callback http_calls_stats;
http_callback http_calls_count;
http_callback http_calls_time;
http_callback http_calls_usage;
http_callback http_calls_version;

http_callback http_calls_json_init;
http_callback http_calls_json_data;
http_callback http_calls_json_done;

// and our global
extern HTTP_CTL *_http;


#endif

