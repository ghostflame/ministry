/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* http.c - invokes microhttpd for serving to prometheus                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "ministry.h"


int http_unused_policy( void *cls, const struct sockaddr *addr, socklen_t addrlen )
{
	return MHD_YES;
}

int http_unused_reader( void *cls, uint64_t pos, char *buf, size_t max )
{
	return -1;
}

void http_unused_reader_free( void *cls )
{
	return;
}

int http_unused_kv( void *cls, enum MHD_ValueKind kind, const char *key, const char *value )
{
	return MHD_NO;
}

int http_unused_post( void *cls, MHD_ValueKind kind, const char *key, const char *filename,
                    const char *content_type, const char *transfer_encoding, const char *data,
                    uint64_t off, size_t size )
{
	return MHD_NO;
}



void *http_log_request( void *cls, const char *uri, HTTP_CONN *conn )
{
	return NULL;
}


void http_log( void *arg, const char *fmt, va_list ap )
{
	char lbuf[4096];

	vsnprintf( lbuf, 4096, fmt, ap );

	info( "[HTTP] %s", lbuf );
}


int http_request_handler( void *cls, HTTP_CONN *conn,
	const char *url, const char *method, const char *version,
	const char *upload_data, size_t upload_data_size,
	void **con_cls )
{

	// we only support GET right now
	if( strcasecmp( method, "GET" ) )
		return MHD_NO;




	return MHD_YES;
}


void http_request_complete( void *cls, HTTP_CONN *conn,
	void **con_cls, HTTP_CODE toe )
{

}







int http_start( void )
{
	CTL_HTTP *h = ctl->http;
	struct in_addr ina;

	if( ! h->enabled )
		return 0;

	h->server = MHD_start_daemon( h->flags, h->port,
			h->cb_ap, NULL,
			h->cb_ah, NULL,
			MHD_OPTION_CONNECTION_LIMIT,        h->conns_max,
			MHD_OPTION_PER_IP_CONNECTION_LIMIT, h->conns_max_ip,
			MHD_OPTION_CONNECTION_TIMEOUT,      h->conns_tmout,
			MHD_OPTION_SOCK_ADDR,				&(h->sin),
			MHD_OPTION_URI_LOG_CALLBACK,		h->cb_rl,
			MHD_OPTION_HTTPS_MEM_KEY,           h->ssl->key,
			MHD_OPTION_HTTPS_MEM_CERT,			h->ssl->cert,
			MHD_OPTION_EXTERNAL_LOGGER,			h->cb_lg,
			MHD_OPTION_END
		);

	return 0;
}


void http_stop( void )
{
	MHD_stop_daemon( ctl->http->server );
}




HTTP_CTL *http_default_config( void )
{
	HTTP_CTL *h;

	h = (HTTP_CTL *) allocz( sizeof( HTTP_CTL ) );

	h->ssl     = (HTTP_SSL *) allocz( sizeof( HTTP_SSL ) );

	h->cb_ah   = &http_request_handler;
	h->cb_ap   = &http_unused_policy;
	h->cb_cr   = &http_unused_reader;
	h->cb_lg   = &http_log;
	h->cb_rc   = &http_request_complete;
	h->cb_rf   = &http_unused_reader_free;
	h->cb_rl   = &http_log_request;

	h->it_kv   = &http_unused_kv;
	h->it_pd   = &http_unused_post;

	h->port    = DEFAULT_HTTP_PORT;
	h->addr    = NULL;
	h->enabled = 0;
	h->stats   = 1;
	h->flags   = MHD_USE_SSL
	            |MHD_USE_THREAD_PER_CONNECTION
	            |MHD_USE_POLL
	            |MHD_USE_PIPE_FOR_SHUTDOWN;

	h->sin.sin_family      = AF_INET;
	h->sin.sin_port        = htons( h->port );
	h->sin.sin_addr.s_addr = INADDR_ANY;


	return h;
}



int httpd_config_line( APV *av )
{
	HTTP_CTL *h = ctl->http;

	if( attIs( "port" ) )
	{
		h->port = (uint16_t) ( strtoul( av->val, NULL, 10 ) & 0xffffUL );
		h->sin.sin_port = htons( h->port );

		debug( "Http port set to %hu.", h->port );
	}
	else if( attIs( "bind" ) )
	{
		if( h->addr )
			free( h->addr );
		h->addr = str_copy( av->val, av->vlen );

		if( !inet_aton( h->addr, &(h->sin.sin_addr) ) )
		{
			err( "Invalid http bind IP address '%s'.", h->addr );
			return -1;
		}

		debug( "Http bind address set to %s.", h->addr );
	}
	else if( attIs( "enable" ) )
	{
		h->enabled = config_bool( av );
		debug( "Http server is %sabled.", ( h->enabled ) ? "en" : "dis" );
	}
	else if( attIs( "stats" ) || attIs( "exposeStats" ) )
	{
		h->stats = config_bool( av );
		debug( "Http exposure of internal stats is %sabled.",
			( h->stats ) ? "en" : "dis" );
	}
	else if( attIs( "maxConns" ) )
	{
		h->conns_max = (unsigned int) strtoul( av->val, NULL, 10 );
		debug( "Http max conns set to %u", h->conns_max );
	}
	else if( attIs( "maxPerIp" ) )
	{
		h->conns_max_ip = (unsigned int) strtoul( av->val, NULL, 10 );
		debug( "Http max conns per IP set to %u", h->conns_max_ip );

	}
	else if( attIs( "timeout" ) || attIs( "tmout" ) )
	{
		h->conns_tmout = (unsigned int) strtoul( av->val, NULL, 10 );
		debug( "Http connection timeout set to %u seconds.", h->conns_tmout );
	}
	else
		return -1;

	return 0;
}


