/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* http.c - invokes microhttpd for serving to prometheus                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "shared.h"


int http_add_handler( char *path, int fixed, http_handler *fp, char *desc, void *arg )
{
	HTPATH *p;
	int len;

	if( !path || !*path || !fp )
	{
		err( "No path or handler function provided." );
		return -1;
	}

	if( *path != '/' )
	{
		err( "Provided path does not begin with /." );
		return -1;
	}

	if( !desc || !*desc )
		desc = "Unknown purpose";

	len = strlen( path );

	for( p = _http->handlers; p; p = p->next )
		if( len == p->plen && !memcmp( path, p->path, len ) )
		{
			err( "Duplicate path handler provided for '%s'.", path );
			return -1;
		}

	p = (HTPATH *) allocz( sizeof( HTPATH ) );
	p->plen = strlen( path );
	p->path = str_dup( path, p->plen );
	p->desc = str_dup( desc, 0 );
	p->fp   = fp;
	p->arg  = arg;

	if( fixed )
	{
		p->blen = fixed;
		p->buf  = (char *) allocz( fixed );
	}

	p->next = _http->handlers;
	_http->handlers = p;
	_http->hdlr_count++;

	debug( "Added url handler: (%hu) %s  ->  %s", _http->hdlr_count, p->path, p->desc );

	return 0;
}


int http_unused_policy( void *cls, const struct sockaddr *addr, socklen_t addrlen )
{
	return MHD_YES;
}

ssize_t http_unused_reader( void *cls, uint64_t pos, char *buf, size_t max )
{
	return -1;
}

void http_unused_reader_free( void *cls )
{
	return;
}

int http_unused_kv( void *cls, HTTP_VAL kind, const char *key, const char *value )
{
	return MHD_NO;
}

int http_unused_post( void *cls, HTTP_VAL kind, const char *key, const char *filename,
                    const char *content_type, const char *transfer_encoding, const char *data,
                    uint64_t off, size_t size )
{
	return MHD_NO;
}



void http_server_panic( void *cls, const char *file, unsigned int line, const char *reason )
{
	err( "[HTTP] Server panic: (%s/%u) %s", file, line, reason );
}



void http_log_request( void *cls, const char *uri, HTTP_CONN *conn )
{
	union MHD_ConnectionInfo *ci;

	// see who we are talking to
	ci = (union MHD_ConnectionInfo *) MHD_get_connection_info( conn, MHD_CONNECTION_INFO_CLIENT_ADDRESS );

	info( "[HTTP] Request {%s}: %s", inet_ntoa( ((struct sockaddr_in *) (ci->client_addr))->sin_addr ), uri );
}


void http_log( void *arg, const char *fm, va_list ap )
{
	char lbuf[4096];

	vsnprintf( lbuf, 4096, fm, ap );

	info( "[HTTP] %s", lbuf );
}

#define _jstr( s )				l += snprintf( b + l, max - l, "%s\r\n", s )
#define _jfielda( p, f )		l += snprintf( b + l, max - l, p "\"%s\": [\r\n", f )
#define _jfieldo( p, f )		l += snprintf( b + l, max - l, p "\"%s\": {\r\n", f )
#define _jfields( p, f, v )		l += snprintf( b + l, max - l, p "\"%s\": \"%s\",\r\n", f, v )
#define _jfieldd( p, f, v )		l += snprintf( b + l, max - l, p "\"%s\": %f,\r\n", f, v )
#define _jfield4( p, f, v )		l += snprintf( b + l, max - l, p "\"%s\": %d,\r\n", f, v )
#define _jfield8( p, f, v )		l += snprintf( b + l, max - l, p "\"%s\": %ld,\r\n", f, v )
#define _jfieldu( p, f, v )		l += snprintf( b + l, max - l, p "\"%s\": %u,\r\n", f, v )
#define _jfieldl( p, f, v )		l += snprintf( b + l, max - l, p "\"%s\": %lu,\r\n", f, v )

int http_handler_stats( uint32_t ip, char **buf, int max, void *arg )
{
	char *b = *buf;
	int j, l = 0;
	MTSTAT ms;

	_jstr( "{" );

	_jfields( "  ", "app", _proc->app_name );
	_jfieldd( "  ", "uptime", get_uptime( ) );
	_jfield8( "  ", "mem", mem_curr_kb( ) );

	_jfielda( "  ", "memtypes" );

	for( j = 0; j < _mem->type_ct; j++ )
	{
		if( mem_type_stats( j, &ms ) != 0 )
			continue;

		_jstr( "    {" );
		_jfields( "      ", "type", ms.name );
		_jfieldu( "      ", "free", ms.freec );
		_jfieldu( "      ", "alloc", ms.alloc );
		_jfieldl( "      ", "kb", ms.bytes >> 10 );

		if( j == ( _mem->type_ct - 1 ) )
			_jstr( "    }" );
		else
			_jstr( "    }," );
	}
	_jstr( "  ]" );
	_jstr( "}" );

	return l;
}

#undef _jstr
#undef _jfielda
#undef _jfieldo
#undef _jfields
#undef _jfieldd
#undef _jfield4
#undef _jfield8
#undef _jfieldu
#undef _jfieldl


int http_handler_usage( uint32_t ip, char **buf, int max, void *arg )
{
	int len = 0;
	HTPATH *p;

	for( p = _http->handlers; p; p = p->next )
		len += snprintf( *buf + len, max - len, "%-16s %s\n",
				p->path, p->desc );

	return len;
}


int http_send_response( char *buf, int len, HTTP_CONN *conn, unsigned int code, int freeBuf )
{
	HTTP_RESP *resp;
	int ret;

	if( !len )
		len = strlen( buf );

	resp = MHD_create_response_from_buffer( len, (void *) buf, MHD_RESPMEM_MUST_COPY );

	if( freeBuf )
		free( buf );

	ret = MHD_queue_response( conn, code, resp );
	MHD_destroy_response( resp );

	return ret;
}



int http_request_handler( void *cls, HTTP_CONN *conn, const char *url,
	const char *method, const char *version, const char *upload_data,
	size_t *upload_data_size, void **con_cls )
{
	int len, blen, rlen, code, doFree;
	union MHD_ConnectionInfo *ci;
	uint32_t ip;
	HTPATH *p;
	char *buf;

	// we only support GET right now
	if( strcasecmp( method, "GET" ) )
		return http_send_response( "Only GET is supported.", 0, conn, MHD_HTTP_METHOD_NOT_ALLOWED, 0 );

	// see who we are talking to
	ci = (union MHD_ConnectionInfo *) MHD_get_connection_info( conn, MHD_CONNECTION_INFO_CLIENT_ADDRESS );
	ip = ((struct sockaddr_in *) (ci->client_addr))->sin_addr.s_addr;

	code = MHD_HTTP_OK;
	rlen = strlen( url );
	len  = 0;

	hit_counter( _http->requests );

	for( p = _http->handlers; p; p = p->next )
	{
		debug( "Comparing url '%s' against path '%s'", url, p->path );
		if( p->plen == rlen && !memcmp( p->path, url, rlen ) )
			break;
	}

	// don't recognise that url
	if( !p )
	{
		buf = "That url is not recognised.\r\n";
		len = strlen( buf );
		code = MHD_HTTP_NOT_FOUND;
		doFree = 0;
	}
	else
	{
		// right - fixed buffer?
		if( ( blen = p->blen ) )
		{
			buf = p->buf;
			doFree = 0;
		}
		// OK, you will make one
		else
		{
			buf = NULL;
			doFree = 1;
		}

		// keep count
		hit_counter( p->hits );

		len = (*(p->fp))( ip, &buf, blen, p->arg );
	}

	return http_send_response( buf, len, conn, code, doFree );
}


void http_request_complete( void *cls, HTTP_CONN *conn,
	void **con_cls, HTTP_CODE toe )
{
	return;
}



int http_ssl_load_file( SSL_FILE *sf, char *type )
{
	char desc[32];

	sf->type = str_dup( type, 0 );

	snprintf( desc, 32, "SSL %s file", sf->type );

	return read_file( sf->path, &(sf->content), &(sf->len), MAX_SSL_FILE_SIZE, 1, desc );
}



int http_ssl_setup( SSL_CONF *s )
{
	if( http_ssl_load_file( &(s->cert), "cert" )
	 || http_ssl_load_file( &(s->key),  "key"  ) )
		return -1;

	return 0;
}




#define mop( _o )		MHD_OPTION_ ## _o

int http_start( void )
{
	HTTP_CTL *h = _http;
	uint16_t port;

	if( !h->enabled )
		return 0;

	if( h->ssl->enabled )
	{
		if( http_ssl_setup( h->ssl ) )
			return -1;

		port = h->ssl->port;
		h->proto = "https";
		flag_add( h->flags, MHD_USE_SSL );
	}
	else
	{
		port = h->port;
		h->proto = "http";
	}

	// you wan to disable this in config if you are writing your own
	if( h->stats )
		http_add_handler( "/stats", h->statsBufSize, &http_handler_stats,  "Internal stats", NULL );

	// reverse the list of handlers
	_http->handlers = (HTPATH *) mem_reverse_list( _http->handlers );

	h->sin->sin_port = htons( port );

	MHD_set_panic_func( h->calls->panic, (void *) h );

	h->server = MHD_start_daemon( h->flags, 0,
			NULL, NULL,
			h->calls->handler,               (void *) h,
			mop( SOCK_ADDR ),                (struct sockaddr *) h->sin,
			mop( URI_LOG_CALLBACK ),         h->calls->reqlog, NULL,
			mop( EXTERNAL_LOGGER ),          h->calls->log, (void *) h,
			mop( CONNECTION_LIMIT ),         h->conns_max,
			mop( PER_IP_CONNECTION_LIMIT ),  h->conns_max_ip,
			mop( CONNECTION_TIMEOUT ),       h->conns_tmout,
			mop( HTTPS_MEM_KEY ),            (const char *) h->ssl->key.content,
			mop( HTTPS_KEY_PASSWORD ),       (const char *) h->ssl->password,
			mop( HTTPS_MEM_CERT ),           (const char *) h->ssl->cert.content,
			mop( END )
		);

	if( !h->server )
	{
		err( "Failed to start %s server on %hu: %s", h->proto, port, Err );
		return -1;
	}

	notice( "Started up %s server on port %hu.", h->proto, port );
	return 0;
}

#undef mop

void http_stop( void )
{
	HTTP_CTL *h = _http;

	if( h->server )
	{
		MHD_stop_daemon( h->server );
		pthread_spin_destroy( &(h->hitlock) );
		notice( "Shut down %s server.", h->proto );
	}
}



HTTP_CTL *http_config_defaults( void )
{
	struct sockaddr_in *sin;
	HTTP_CTL *h;

	h                  = (HTTP_CTL *) allocz( sizeof( HTTP_CTL ) );
	h->ssl             = (SSL_CONF *) allocz( sizeof( SSL_CONF ) );
	h->calls           = (HTTP_CB *) allocz( sizeof( HTTP_CB ) );

	h->conns_max       = DEFAULT_HTTP_CONN_LIMIT;
	h->conns_max_ip    = DEFAULT_HTTP_CONN_IP_LIMIT;
	h->conns_tmout     = DEFAULT_HTTP_CONN_TMOUT;
	h->port            = DEFAULT_HTTP_PORT;
	h->addr            = NULL;
	h->stats           = 1;
	//h->flags           = MHD_USE_THREAD_PER_CONNECTION|MHD_USE_POLL|MHD_USE_DEBUG|MHD_USE_ERROR_LOG;
	h->flags           = MHD_USE_THREAD_PER_CONNECTION|MHD_USE_POLL;

	h->enabled         = 0;
	h->ssl->enabled    = 0;

	h->calls->handler  = &http_request_handler;
	h->calls->panic    = &http_server_panic;
	h->calls->policy   = &http_unused_policy;
	h->calls->reader   = &http_unused_reader;
	h->calls->complete = &http_request_complete;
	h->calls->rfree    = &http_unused_reader_free;

	h->calls->kv       = &http_unused_kv;
	h->calls->post     = &http_unused_post;

	h->calls->log      = &http_log;
	h->calls->reqlog   = &http_log_request;

	// lock for hits counters
	pthread_spin_init( &(h->hitlock), PTHREAD_PROCESS_PRIVATE );

	// make the address binding structure
	sin = (struct sockaddr_in *) allocz( sizeof( struct sockaddr_in ) );
	sin->sin_family      = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;
	h->sin               = sin;
	h->statsBufSize      = DEFAULT_STATS_BUF_SZ;

	_http = h;

	http_add_handler( "/",  8192, &http_handler_usage,  "Usage information", NULL );

	return _http;
}


#define dIs( _str )		!strcasecmp( d, _str )

int http_config_line( AVP *av )
{
	HTTP_CTL *h = _http;
	char *d;

	if( !( d = strchr( av->att, '.' ) ) )
	{
		if( attIs( "port" ) )
		{
			h->port = (uint16_t) ( strtoul( av->val, NULL, 10 ) & 0xffffUL );
			debug( "Http port set to %hu.", h->port );
		}
		else if( attIs( "bind" ) )
		{
			if( h->addr )
				free( h->addr );
			h->addr = str_copy( av->val, av->vlen );

			if( !inet_aton( h->addr, &(h->sin->sin_addr) ) )
			{
				err( "Invalid http server bind IP address '%s'.", h->addr );
				return -1;
			}

			debug( "Http server bind address set to %s.", h->addr );
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
		else if( attIs( "statsBufSize" ) )
		{
			av_int( h->statsBufSize );
			if( h->statsBufSize <= 0 )
				h->statsBufSize = DEFAULT_STATS_BUF_SZ;

			debug( "Http stats buf size is now %ld.", h->statsBufSize );
		}
		else
			return -1;

		return 0;
	}

	// then its conns. or ssl.
	d++;

	if( !strncasecmp( av->att, "conns.", 6 ) )
	{
		if( dIs( "max" ) )
		{
			h->conns_max = (unsigned int) strtoul( av->val, NULL, 10 );
		}
		else if( dIs( "maxPerIp" ) || dIs( "maxPerHost" ) )
		{
			h->conns_max_ip = (unsigned int) strtoul( av->val, NULL, 10 );
		}
		else if( dIs( "timeout" ) || dIs( "tmout" ) )
		{
			h->conns_tmout = (unsigned int) strtoul( av->val, NULL, 10 );
		}
		else
			return -1;
	}
	else if( !strncasecmp( av->att, "ssl.", 4 ) )
	{
		if( dIs( "certFile" ) || dIs( "cert" ) )
		{
			if( h->ssl->cert.path )
				free( h->ssl->cert.path );
			h->ssl->cert.path = str_copy( av->val, av->vlen );
		}
		else if( dIs( "keyFile" ) || dIs( "key" ) )
		{
			if( h->ssl->key.path )
				free( h->ssl->key.path );
			h->ssl->key.path = str_copy( av->val, av->vlen );
		}
		else if( dIs( "keyPass" ) || dIs( "pass" ) || dIs( "password" ) )
		{
			if( h->ssl->password )
			{
				free( h->ssl->password );
				h->ssl->password = NULL;
			}
			
			if( !valIs( "null" ) && !valIs( "-" ) )
				h->ssl->password = str_copy( av->val, av->vlen );
		}
		else if( dIs( "enable" ) )
		{
			h->ssl->enabled = config_bool( av );

			if( h->ssl->enabled )
				debug( "SSL enabled on http server." );
		}
		else if( dIs( "port" ) )
		{
			h->ssl->port = (uint16_t) strtoul( av->val, NULL, 10 );
			debug( "SSL port set to %hu.", h->ssl->port );
		}
	}
	else
		return -1;

	return 0;
}

#undef dIs

