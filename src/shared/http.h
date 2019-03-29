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

#define DEFAULT_STATS_BUF_SZ			8192

#define MAX_TLS_FILE_SIZE				65536
#define MAX_TLS_PASS_SIZE				512


enum http_method_shortcuts
{
	HTTP_METH_GET = 0,
	HTTP_METH_POST = 1,
	HTTP_METH_OTHER,
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

	MHD_KeyValueIterator					kv;
	MHD_PostDataIterator					post;
};


struct http_post_state
{
	void				*	obj;

	const char			*	data;	// this post
	size_t					bytes;	// this post size

	size_t					total;

	int						calls;
	int						valid;
};

struct http_req_data
{
	HTREQ				*	next;
	HTPATH				*	path;
	HTTP_CONN			*	conn;
	BUF					*	text;
	struct sockaddr_in	*	sin;
	HTTP_POST			*	post;
	int						code;
	int						meth;
	int						err;
	int						sent;
	int						first;
};



struct http_path
{
	HTPATH				*	next;
	HTHDLS				*	list;
	http_callback		*	req;
	http_callback		*	init;
	http_callback		*	fini;
	char				*	path;
	char				*	desc;
	void				*	arg;
	int64_t					hits;
	int						plen;
};


struct http_handlers
{
	HTPATH				*	list;
	char				*	method;
	int						count;
};


#define lock_hits( )		pthread_spin_lock(   &(_http->hitlock) )
#define unlock_hits( )		pthread_spin_unlock( &(_http->hitlock) )

#define hit_counter( _p )	lock_hits( ); (_p)++; unlock_hits( )

struct http_control
{
	//struct MHD_Daemon	*	server;
	HTTP_SVR			*	server;

	HTTP_CB				*	calls;
	TLS_CONF			*	tls;

	HTHDLS				*	get_h;
	HTHDLS				*	post_h;

	unsigned int			flags;
	unsigned int			hflags;
	unsigned int			conns_max;
	unsigned int			conns_max_ip;
	unsigned int			conns_tmout;

	struct sockaddr_in	*	sin;

	pthread_spinlock_t		hitlock;

	uint16_t				hdlr_count;

	uint16_t				port;
	char				*	addr;
	char				*	proto;

	int64_t					requests;
	int64_t					statsBufSize;

	int8_t					enabled;
	int8_t					stats;
};


sort_fn __http_cmp_handlers;

int http_add_handler( char *path, char *desc, void *arg, int method, http_callback *fp, http_callback *init, http_callback *fini );

int http_start( void );
void http_stop( void );

int http_ask_password( void );

HTTP_CTL *http_config_defaults( void );
int http_config_line( AVP *av );

#endif

