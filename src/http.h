/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* http.h - defines HTTP structures and functions                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_HTTP_H
#define MINISTRY_HTTP_H


typedef cb_ContentReader     MHD_ContentReaderCallback;
typedef cb_ContentReaderFree MHD_ContentReaderFreeCallback;
typedef cb_AccessPolicy      MHD_AccessPolicyCallback;
typedef cb_AccessHandler     MHD_AccessHandlerCallback;
typedef cb_RequestCompleted  MHD_RequestCompletedCallback;
typedef it_KeyValue          MHD_KeyValueIterator;
typedef it_PostData          MHD_PostDataIterator;

typedef HTTP_CONN struct MHD_Connection;
typedef HTTP_CODE enum MHD_RequestTerminationCode;

typedef void *cb_RequestLogger ( void *cls, const char *uri, HTTP_CONN *conn );
typedef void *cb_Logger        ( void *arg, const char *fmt, va_list ap );


cb_AccessHandler http_request_handler;
cb_RequestCompleted http_request_complete;
cb_AccessPolicy http_unused_policy;
cb_ContentReader http_unused_reader;
cb_ContentReaderFree http_unused_reader_free;
cb_RequestLogger http_log_request;
cb_Logger http_log;
it_KeyValue http_unused_kv;
it_PostData http_unused_post;


#define HTTP_DEFAULT_CONN_LIMIT			256
#define HTTP_DEFAULT_CONN_IP_LIMIT		64
#define HTTP_DEFAULT_CONN_TMOUT			10


struct http_ssl
{
	const char				*	key;
	const char				*	cert;
};



struct http_control
{
	MHD_Daemon				*	server;
	cb_AccessHandler		*	cb_ah;
	cb_AccessPolicy			*	cb_ap;
	cb_ContentReader		*	cb_cr;
	cb_Logger				*	cb_lg;
	cb_RequestCompleted		*	cb_rc;
	cb_ContentReaderFree	*   cb_rf;
	cb_RequestLogger		*	cb_rl;
	it_KeyValue				*	it_kv;
	it_PostData				*	it_pd;

	unsigned int				flags;
	unsigned int				conns_max;
	unsigned int				conns_max_ip;
	unsigned int				conns_tmout;

	struct sockaddr_in			sin;

	uint16_t					port;
	char					*	addr;

	HTTP_SSL				*	ssl;

	int							enabled;
	int							stats;
	int							sock;
};






throw_fn http_loop;

HTTP_CTL *http_default_config( void );
int http_config_line( AVP *av );

#endif
