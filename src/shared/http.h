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

typedef void (*cb_RequestLogger) ( void *cls, const char *uri, HTTP_CONN *conn );


#define DEFAULT_HTTP_PORT				9080
#define DEFAULT_HTTPS_PORT              9443

#define DEFAULT_HTTP_CONN_LIMIT			256
#define DEFAULT_HTTP_CONN_IP_LIMIT		64
#define DEFAULT_HTTP_CONN_TMOUT			10

#define DEFAULT_STATS_BUF_SZ			8192

#define MAX_SSL_FILE_SIZE				65536
#define MAX_SSL_PASS_SIZE				512


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
	volatile char			*	password;
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


struct http_path
{
	HTPATH				*	next;
	http_handler		*	fp;
	char				*	path;
	char				*	desc;
	char				*	buf;
	void				*	arg;
	int64_t					hits;
	int						plen;
	int						blen;
};


#define lock_hits( )		pthread_spin_lock(   &(_http->hitlock) )
#define unlock_hits( )		pthread_spin_unlock( &(_http->hitlock) )

#define hit_counter( _p )	lock_hits( ); (_p)++; unlock_hits( )

struct http_control
{
	//struct MHD_Daemon	*	server;
	HTTP_SVR			*	server;

	HTTP_CB				*	calls;
	SSL_CONF			*	ssl;

	HTPATH				*	handlers;

	unsigned int			flags;
	unsigned int            hflags;
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



int http_add_handler( char *path, int fixed, http_handler *fp, char *desc, void *arg );

int http_start( void );
void http_stop( void );

int http_ask_password( void );

HTTP_CTL *http_config_defaults( void );
int http_config_line( AVP *av );

#endif

