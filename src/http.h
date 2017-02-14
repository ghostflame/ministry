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
typedef struct MDH_PostProcessor        HTTP_POST;

typedef void (*cb_RequestLogger) ( void *cls, const char *uri, HTTP_CONN *conn );




#define DEFAULT_HTTP_CONN_LIMIT			256
#define DEFAULT_HTTP_CONN_IP_LIMIT		64
#define DEFAULT_HTTP_CONN_TMOUT			10

#define MAX_SSL_FILE_SIZE				65536

#define DEFAULT_HTTP_MAX_RESP			1024


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

struct url_map_data
{
	int							id;
	url_fn					*	fp;
	int							post;
	int							len;
	char					*	url;
	char					*	desc;
};


struct http_response
{
	RESP					*	next;
	URL						*	url;
	BUF						*	buf;

	const char				*	data;
	struct sockaddr_in			peer;

	HOST						host;
	size_t						dlen;

	HTTP_CODE					code;

	int8_t						meth;
	int8_t						had_post;
};


enum url_method_map
{
	METHOD_UNKNOWN = 0,
	METHOD_GET,
	METHOD_POST,
	METHOD_MAX
};


enum url_map_id
{
	URL_ID_DISALLOWED = 0,
	URL_ID_UNKNOWN,
	URL_ID_SLASH,
	URL_ID_STATUS,
	URL_ID_TOKENS,
	URL_ID_STATS,
	URL_ID_ADDER,
	URL_ID_GAUGE,
	URL_ID_COMPAT,
//	URL_ID_METRICS,
	URL_ID_MAX
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

	URL					*	map;
	URL					*	disallowed;
	URL					*	unknown;


	unsigned int			flags;
	unsigned int			conns_max;
	unsigned int			conns_max_ip;
	unsigned int			conns_tmout;

	struct sockaddr_in	*	sin;

	uint16_t				port;
	char				*	addr;
	char				*	proto;

	int						enabled;
	int						status;

	int32_t					max_resp;
};


url_fn http_handler_disallowed;
url_fn http_handler_unknown;
url_fn http_handler_usage;
url_fn http_handler_tokens;
url_fn http_handler_status;
url_fn http_handler_data_stats;
url_fn http_handler_data_adder;
url_fn http_handler_data_gauge;
url_fn http_handler_data_compat;



int http_start( void );
void http_stop( void );

void http_log( void *arg, const char *fm, va_list ap );

HTTP_CTL *http_config_defaults( void );
int http_config_line( AVP *av );

#endif

