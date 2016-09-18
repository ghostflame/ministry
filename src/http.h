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


typedef struct MHD_Daemon               HTTP_SVR;
typedef struct MHD_Connection           HTTP_CONN;
typedef enum MHD_RequestTerminationCode HTTP_CODE;
typedef enum MHD_ValueKind              HTTP_VAL;
typedef struct MHD_Response             HTTP_RESP;

typedef void (*cb_RequestLogger) ( void *cls, const char *uri, HTTP_CONN *conn );


#define DEFAULT_HTTP_CONN_LIMIT			256
#define DEFAULT_HTTP_CONN_IP_LIMIT		64
#define DEFAULT_HTTP_CONN_TMOUT			10

#define MAX_SSL_FILE_SIZE				65536


struct http_ssl_file
{
	const char				*	type;
	char					*	content;
	char					*	path;
	int							len;
};


struct http_ssl
{
	SSL_FILE					key;
	SSL_FILE					cert;
	char					*	password;
	int							enabled;

	uint16_t					port;
};


struct http_callbacks
{
	MHD_PanicCallback						panic;
	MHD_AcceptPolicyCallback				policy;
	MHD_AccessHandlerCallback				handler;
	MHD_RequestCompletedCallback			complete;
	MHD_ContentReaderCallback				reader;
	MHD_ContentReaderFreeCallback			rfree;

	MHD_LogCallback							log;
	cb_RequestLogger						reqlog;

	MHD_KeyValueIterator					kv;
	MHD_PostDataIterator					post;
};



struct http_control
{
	//struct MHD_Daemon	*	server;
	HTTP_SVR			*	server;

	HTTP_CB				*	calls;
	SSL_CONF			*	ssl;

	unsigned int			flags;
	unsigned int			conns_max;
	unsigned int			conns_max_ip;
	unsigned int			conns_tmout;

	struct sockaddr_in	*	sin;

	uint16_t				port;
	char				*	addr;
	char				*	proto;

	int						enabled;
	int						stats;
};



int http_start( void );
void http_stop( void );


HTTP_CTL *http_config_defaults( void );
int http_config_line( AVP *av );

#endif

