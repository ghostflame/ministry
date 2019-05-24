/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* http.h - defines HTTP structures and functions                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_HTTP_H
#define SHARED_HTTP_H




typedef struct MHD_Daemon               HTTP_SVR;
typedef struct MHD_Connection           HTTP_CONN;
typedef enum MHD_RequestTerminationCode HTTP_CODE;
typedef enum MHD_ValueKind              HTTP_VAL;
typedef struct MHD_Response             HTTP_RESP;
typedef struct MHD_PostProcessor		HTTP_PPROC;

typedef void (*cb_RequestLogger) ( void *cls, const char *uri, HTTP_CONN *conn );


#define DEFAULT_HTTP_PORT				9080
#define DEFAULT_HTTPS_PORT              9443

#define DEFAULT_HTTP_CONN_LIMIT			256
#define DEFAULT_HTTP_CONN_IP_LIMIT		64
#define DEFAULT_HTTP_CONN_TMOUT			10

#define DEFAULT_POST_BUF_SZ				0x4000	// 16k

#define MAX_TLS_FILE_SIZE				0x10000	// 64k
#define MAX_TLS_PASS_SIZE				512



#define HTTP_FLAGS_CONTROL				0x0001
#define HTTP_FLAGS_JSON					0x0002
#define HTTP_FLAGS_NO_REPORT			0x0100



enum http_method_shortcuts
{
	HTTP_METH_GET = 0,
	HTTP_METH_POST = 1,
	HTTP_METH_OTHER,
};


enum http_post_states
{
	HTTP_POST_START = 0,
	HTTP_POST_BODY,
	HTTP_POST_END,
};


struct http_tls_file
{
	const char				*	type;
	char					*	content;
	char					*	path;
	int							len;
};


struct http_tls
{
	TLS_FILE					key;
	TLS_FILE					cert;
	volatile char			*	password;
	char					*	priorities;
	int							enabled;

	uint16_t					port;
	uint16_t					passlen;
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
};


struct http_post_state
{
	void				*	obj;
	void				*	objFree;// this object will be freed by post handler

	const char			*	data;	// this post
	size_t					bytes;	// this post size

	AVP						kv;		// key-value for post-processor

	size_t					total;

	int						calls;
	int						state;	// where in the post cycle are we?
};

#define HTTP_CLS_CHECK		0xdeadbeefdeadbeef

struct http_req_data
{
	HTREQ				*	next;
	uint64_t				check;
	int64_t					start;
	HTPATH				*	path;
	HTTP_CONN			*	conn;
	BUF					*	text;
	struct sockaddr_in		sin;
	HTTP_POST			*	post;
	HTTP_PPROC			*	pproc;
	int						code;
	int						meth;
	int						err;
	int						sent;
	int						first;
};


struct http_path
{
	HTPATH				*	next;

	http_callback		*	cb;

	IPLIST				*	srcs;
	HTHDLS				*	list;

	char				*	path;
	char				*	desc;
	char				*	iplist;
	void				*	arg;

	int64_t					hits;
	int						plen;
	int						flags;
};


struct http_handlers
{
	HTPATH				*	list;
	char				*	method;
	int						count;
};


#define lock_hits( )		pthread_mutex_lock(   &(_http->hitlock) )
#define unlock_hits( )		pthread_mutex_unlock( &(_http->hitlock) )

#define hit_counter( _p )	lock_hits( ); (_p)++; unlock_hits( )

struct http_control
{
	//struct MHD_Daemon	*	server;
	HTTP_SVR			*	server;

	HTTP_CB				*	calls;
	TLS_CONF			*	tls;

	HTHDLS				*	get_h;
	HTHDLS				*	post_h;

	IPLIST				*	web_ips;
	IPLIST				*	ctl_ips;

	unsigned int			flags;
	unsigned int			hflags;
	unsigned int			conns_max;
	unsigned int			conns_max_ip;
	unsigned int			conns_tmout;

	struct sockaddr_in	*	sin;

	http_callback		*	stats_fp;   // extra stats callback

	http_reporter		*	rpt_fp;
	void				*	rpt_arg;

	pthread_mutex_t			hitlock;

	uint16_t				hdlr_count;

	uint16_t				port;
	uint16_t				server_port; // what we end up using
	char				*	addr;
	char				*	proto;

	int64_t					requests;

	int8_t					enabled;
};


sort_fn __http_cmp_handlers;

int http_add_handler( char *path, char *desc, void *arg, int method, http_callback *fp, IPLIST *srcs, int flags );
int http_add_control( char *path, char *desc, void *arg, http_callback *fp, IPLIST *srcs, int flags );

int http_stats_handler( http_callback *fp );

// give us a simple call to add get handlers
#define http_add_simple_get( _p, _d, _c )			http_add_handler( _p, _d, NULL, HTTP_METH_GET, _c, NULL, 0 )
#define http_add_json_get( _p, _d, _c )				http_add_handler( _p, _d, NULL, HTTP_METH_GET, _c, NULL, HTTP_FLAGS_JSON )

void http_set_reporter( http_reporter *fp, void *arg );

int http_start( void );
void http_stop( void );

int http_ask_password( void );

HTTP_CTL *http_config_defaults( void );
int http_config_line( AVP *av );

#endif

