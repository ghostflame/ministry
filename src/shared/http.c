/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* http.c - invokes microhttpd for serving to prometheus                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "shared.h"

#ifndef MIN_MHD_PASS
  #if MHD_VERSION > 0x00094000
    #define MIN_MHD_PASS 1
  #else
    #define MIN_MHD_PASS 0
  #endif
#endif

HTTP_CTL *_http = NULL;



HTPATH *http_find_callback( const char *url, int rlen, HTHDLS *hd )
{
	HTPATH *p;

	for( p = hd->list; p; p = p->next )
	{
		//debug( "Comparing url '%s' against path '%s'", url, p->path );
		if( p->plen == rlen && !memcmp( p->path, url, rlen ) )
			return p;
	}

	return NULL;
}


int http_add_handler( char *path, char *desc, void *arg, int method, http_callback *fp, http_callback *init, http_callback *fini )
{
	HTHDLS *hd;
	HTPATH *p;
	int len;

	if( method == HTTP_METH_GET )
		hd = _http->get_h;
	else if( method == HTTP_METH_POST )
		hd = _http->post_h;
	else
	{
		err( "Only GET and POST are supported (%d - %s)", method, path );
		return -1;
	}

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

	if( http_find_callback( path, len, hd ) )
	{
		err( "Duplicate path (%s) handler provided for '%s'.", hd->method, path );
		return -1;
	}

	p = (HTPATH *) allocz( sizeof( HTPATH ) );
	p->plen  = len;
	p->path  = str_dup( path, len );
	p->desc  = str_dup( desc, 0 );
	p->arg   = arg;
	p->list  = hd;

	p->req   = fp;
	p->init  = init;
	p->fini  = fini;

	p->next  = hd->list;
	hd->list = p;

	hd->count++;
	_http->hdlr_count++;

	// re-sort that list
	mem_sort_list( (void **) &(hd->list), hd->count, __http_cmp_handlers );
	debug( "Added %s handler: (%d/%hu) %s  ->  %s", hd->method, hd->count, _http->hdlr_count, p->path, p->desc );

	return 0;
}





int http_unused_policy( void *cls, const struct sockaddr *addr, socklen_t addrlen )
{
	info( "Called: http_unused_policy." );
	return MHD_YES;
}

ssize_t http_unused_reader( void *cls, uint64_t pos, char *buf, size_t max )
{
	info( "Called: http_unused_reader." );
	return -1;
}

void http_unused_reader_free( void *cls )
{
	info( "Called: http_unused_reader_free." );
	return;
}

int http_unused_kv( void *cls, HTTP_VAL kind, const char *key, const char *value )
{
	info( "Called: http_unused_kv." );
	return MHD_NO;
}

int http_unused_post( void *cls, HTTP_VAL kind, const char *key, const char *filename,
                      const char *content_type, const char *transfer_encoding, const char *data,
                      uint64_t off, size_t size )
{
	info( "Called: http_unused_post." );
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

#define _jstr( s )				strbuf_aprintf( b, "%s\r\n", s )
#define _jfielda( p, f )		strbuf_aprintf( b, p "\"%s\": [\r\n", f )
#define _jfieldo( p, f )		strbuf_aprintf( b, p "\"%s\": {\r\n", f )
#define _jfields( p, f, v )		strbuf_aprintf( b, p "\"%s\": \"%s\",\r\n", f, v )
#define _jfieldd( p, f, v )		strbuf_aprintf( b, p "\"%s\": %f,\r\n", f, v )
#define _jfield4( p, f, v )		strbuf_aprintf( b, p "\"%s\": %d,\r\n", f, v )
#define _jfield8( p, f, v )		strbuf_aprintf( b, p "\"%s\": %ld,\r\n", f, v )
#define _jfieldu( p, f, v )		strbuf_aprintf( b, p "\"%s\": %u,\r\n", f, v )
#define _jfieldl( p, f, v )		strbuf_aprintf( b, p "\"%s\": %lu,\r\n", f, v )
#define _jprunecma( )			if( strbuf_lastchar( b ) == '\n' ) { strbuf_chopn( b, 3 ); }

#define _jmtype( n, mc )		snprintf( tmp, 16, "%s_calls", n ); \
								_jfieldl( "      ", tmp, mc.ctr ); \
								snprintf( tmp, 16, "%s_total", n ); \
								_jfieldl( "      ", tmp, mc.sum )


int http_handler_stats( HTREQ *req )
{
#ifdef MTYPE_TRACING
	char tmp[16];
#endif
	MTSTAT ms;
	BUF *b;
	int j;

	b = ( req->text = strbuf_resize( req->text, _http->statsBufSize ) );

	_jstr( "{" );

	_jfields( "  ", "app", _proc->app_name );
	_jfieldd( "  ", "uptime", get_uptime( ) );
	_jfield8( "  ", "mem", mem_curr_kb( ) );

	_jfielda( "  ", "memtypes" );

	for( j = 0; j < _proc->mem->type_ct; j++ )
	{
		if( mem_type_stats( j, &ms ) != 0 )
			continue;

		_jstr( "    {" );
		_jfields( "      ", "type", ms.name );
		_jfieldu( "      ", "free", ms.ctrs.fcount );
		_jfieldu( "      ", "alloc", ms.ctrs.total );
		_jfieldl( "      ", "kb", ms.bytes >> 10 );
#ifdef MTYPE_TRACING
		_jmtype( "alloc",  ms.ctrs.all );
		_jmtype( "freed",  ms.ctrs.fre );
		_jmtype( "preall", ms.ctrs.pre );
		_jmtype( "refill", ms.ctrs.ref );
#endif
		_jprunecma( );
		_jstr( "    }," );
	}
	_jprunecma( );
	_jstr( "  ]" );
	_jstr( "}" );

	return 0;
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
#undef _jprunecma


int __http_cmp_handlers( const void *h1, const void *h2 )
{
	HTPATH *p1 = *((HTPATH **) h1);
	HTPATH *p2 = *((HTPATH **) h2);

	return ( p1->plen < p2->plen ) ? -1 : ( p1->plen == p2->plen ) ? 0 : 1;
}


void __http_handler_usage_type( HTHDLS *hd, BUF *b )
{
	HTPATH *p;

	if( hd->count )
	{
		strbuf_aprintf( b, "[%s]\n", hd->method );

		for( p = hd->list; p; p = p->next )
			strbuf_aprintf( b, "%-16s %s\n", p->path, p->desc );
	}
}


int http_handler_usage( HTREQ *req )
{
	req->text = strbuf_resize( req->text, 8100 );
	strbuf_empty( req->text );

	__http_handler_usage_type( _http->get_h,  req->text );
	__http_handler_usage_type( _http->post_h, req->text );

	return 0;
}


int http_send_response( HTREQ *req )
{
	int ret = MHD_YES;
	HTTP_RESP *resp;
	BUF *b;

	if( req->sent )
	{
		notice( "http_send_response called twice." );
		return ret;
	}

	if( ( b = req->text ) )
	{
		resp = MHD_create_response_from_buffer( b->len, (void *) b->buf, MHD_RESPMEM_MUST_COPY );
		strbuf_empty( b );
	}
	else
	{
		resp = MHD_create_response_from_buffer( 0, "", MHD_RESPMEM_MUST_COPY );
	}

	ret = MHD_queue_response( req->conn, req->code, resp );
	MHD_destroy_response( resp );

	req->sent = 1;

	return ret;
}




void http_request_complete( void *cls, HTTP_CONN *conn,
	void **arg, HTTP_CODE toe )
{
	HTREQ *req;

	if( !( req = (HTREQ *) *arg ) )
		return;

	//debug( "Complete with req arg value as %p.", req );

	if( req->sent == 0 )
		http_send_response( req );

	if( req->post && req->post->valid )
		(req->path->fini)( req );

	mem_free_request( &req );

	*arg = NULL;
}


int http_request_handler( void *cls, HTTP_CONN *conn, const char *url,
	const char *method, const char *version, const char *upload_data,
	size_t *upload_data_size, void **con_cls )
{
	union MHD_ConnectionInfo *ci;
	HTREQ *req;
	HTHDLS *hd;
	int rlen;

	// WHISKEY TANGO FOXTROT, libmicrohttpd?
	if( ((int64_t) *con_cls) < 0x1ff )
	{
		//debug( "Flattening bizarre arg value %p.", *con_cls );
		*con_cls = NULL;
	}

	// do we have an object?
	if( !( req = (HTREQ *) *con_cls ) )
	{
		req = mem_new_request( );
		req->conn = conn;
		req->code = MHD_HTTP_OK;

		*con_cls = req;

		rlen = strlen( url );

		if( !strcasecmp( method, MHD_HTTP_METHOD_GET ) )
		{
			req->meth = HTTP_METH_GET;
			hd = _http->get_h;
		}
		else if( !strcasecmp( method, MHD_HTTP_METHOD_POST ) )
		{
			req->meth = HTTP_METH_POST;
			hd = _http->post_h;
		}
		else
		{
			req->text = strbuf_create( "Method not supported.", 0 );
			req->code = MHD_HTTP_METHOD_NOT_ALLOWED;
			http_send_response( req );
			return MHD_YES;
		}

		// find the handler
		if( !( req->path = http_find_callback( url, rlen, hd ) ) )
		{
			req->text = strbuf_create( "That url is not recognised.", 0 );
			req->code = MHD_HTTP_NOT_FOUND;
			http_send_response( req );
			return MHD_YES;
		}

		hit_counter( req->path->hits );

		ci = (union MHD_ConnectionInfo *) MHD_get_connection_info( conn, MHD_CONNECTION_INFO_CLIENT_ADDRESS );
		req->sin = (struct sockaddr_in *) (ci->client_addr);

		if( req->meth == HTTP_METH_POST )
		{
			// and create the post data object
			req->post = (HTTP_POST *) allocz( sizeof( HTTP_POST ) );
			(req->path->init)( req );
			req->first = 1;
		}
	}

	switch( req->meth )
	{
		case HTTP_METH_GET:
			// gets are easy
			(req->path->req)( req );
			http_send_response( req );
			return MHD_YES;

		case HTTP_METH_POST:
			if( req->first )
			{
				req->first = 0;
				return MHD_YES;
			}
			req->post->data   = upload_data;
			req->post->bytes  = *upload_data_size;
			req->post->total += req->post->bytes;
			req->post->calls++;

			if( req->post->bytes )
			{
				if( (req->path->req)( req ) < 0 )
					req->err = 1;
			}
			else
			{
				notice( "All done, sending response." );
				http_send_response( req );
			}

			// say we have processed the data
			*upload_data_size = 0;

			return MHD_YES;
	}

	warn( "How did the request logic get here?" );
	return MHD_NO;
}




int http_ssl_load_file( SSL_FILE *sf, char *type )
{
	char desc[32];

	sf->type = str_dup( type, 0 );

	snprintf( desc, 32, "SSL %s file", sf->type );

	sf->len = MAX_SSL_FILE_SIZE;

	return read_file( sf->path, &(sf->content), &(sf->len), 1, desc );
}



int http_ssl_setup( SSL_CONF *s )
{
	if( http_ssl_load_file( &(s->cert), "cert" )
	 || http_ssl_load_file( &(s->key),  "key"  ) )
		return -1;

	return 0;
}



// stomp on our password/length
void http_clean_password( HTTP_CTL *h )
{
	if( h->ssl->password )
	{
		if( h->ssl->passlen )
			memset( (void *) h->ssl->password, 0, h->ssl->passlen );

		free( (void *) h->ssl->password );
	}

	h->ssl->password = NULL;
	h->ssl->passlen = 0;
}



#define mop( _o )		MHD_OPTION_ ## _o

int http_start( void )
{
	HTTP_CTL *h = _http;
	uint16_t port;

	if( !h->enabled )
	{
		http_clean_password( h );
		return 0;
	}

	if( h->ssl->enabled )
	{
		if( http_ssl_setup( h->ssl ) )
		{
			err( "Failed to load TLS certificates." );
			http_clean_password( h );
			return -1;
		}

		port = h->ssl->port;
		h->proto = "https";
		flag_add( h->flags, MHD_USE_SSL );
	}
	else
	{
		port = h->port;
		h->proto = "http";
	}

	// you want to disable this in config if you are writing your own
	if( h->stats )
		http_add_handler( "/stats", "Internal stats", NULL, HTTP_METH_GET, &http_handler_stats, NULL, NULL );

	h->sin->sin_port = htons( port );

	MHD_set_panic_func( h->calls->panic, (void *) h );

	h->server = MHD_start_daemon( h->flags, 0,
			NULL, NULL,
			h->calls->handler,               (void *) h,
			mop( SOCK_ADDR ),                (struct sockaddr *) h->sin,
			mop( URI_LOG_CALLBACK ),         h->calls->reqlog, NULL,
			mop( EXTERNAL_LOGGER ),          h->calls->log, (void *) h,
			mop( NOTIFY_COMPLETED ),         h->calls->complete, NULL,
			mop( CONNECTION_LIMIT ),         h->conns_max,
			mop( PER_IP_CONNECTION_LIMIT ),  h->conns_max_ip,
			mop( CONNECTION_TIMEOUT ),       h->conns_tmout,
			mop( HTTPS_MEM_KEY ),            (const char *) h->ssl->key.content,
#if MIN_MHD_PASS > 0
			mop( HTTPS_KEY_PASSWORD ),       (const char *) h->ssl->password,
#endif
			mop( HTTPS_MEM_CERT ),           (const char *) h->ssl->cert.content,
			mop( END )
		);

	// clean up the password variable
	http_clean_password( h );

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
	h->flags           = MHD_USE_THREAD_PER_CONNECTION|MHD_USE_POLL|MHD_USE_DEBUG|MHD_USE_ERROR_LOG;
	//h->flags           = MHD_USE_THREAD_PER_CONNECTION|MHD_USE_POLL;

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

	// and handler structures
	h->get_h          = (HTHDLS *) allocz( sizeof( HTHDLS ) );
	h->get_h->method  = "GET";
	h->post_h         = (HTHDLS *) allocz( sizeof( HTHDLS ) );
	h->post_h->method = "POST";

	// and our default - help
	http_add_handler( "/", "Usage information", NULL, HTTP_METH_GET, &http_handler_usage, NULL, NULL );

	return _http;
}


#define dIs( _str )		!strcasecmp( d, _str )

int http_config_line( AVP *av )
{
	HTTP_CTL *h = _http;
	char *d;

	if( !( d = strchr( av->aptr, '.' ) ) )
	{
		if( attIs( "port" ) )
		{
			h->port = (uint16_t) ( strtoul( av->vptr, NULL, 10 ) & 0xffffUL );
			debug( "Http port set to %hu.", h->port );
		}
		else if( attIs( "bind" ) )
		{
			if( h->addr )
				free( h->addr );
			h->addr = str_copy( av->vptr, av->vlen );

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

	if( !strncasecmp( av->aptr, "conns.", 6 ) )
	{
		if( dIs( "max" ) )
		{
			h->conns_max = (unsigned int) strtoul( av->vptr, NULL, 10 );
		}
		else if( dIs( "maxPerIp" ) || dIs( "maxPerHost" ) )
		{
			h->conns_max_ip = (unsigned int) strtoul( av->vptr, NULL, 10 );
		}
		else if( dIs( "timeout" ) || dIs( "tmout" ) )
		{
			h->conns_tmout = (unsigned int) strtoul( av->vptr, NULL, 10 );
		}
		else
			return -1;
	}
	else if( !strncasecmp( av->aptr, "ssl.", 4 ) )
	{
		if( dIs( "certFile" ) || dIs( "cert" ) )
		{
			if( h->ssl->cert.path )
				free( h->ssl->cert.path );
			h->ssl->cert.path = str_copy( av->vptr, av->vlen );
		}
		else if( dIs( "keyFile" ) || dIs( "key" ) )
		{
			if( h->ssl->key.path )
				free( h->ssl->key.path );
			h->ssl->key.path = str_copy( av->vptr, av->vlen );
		}
		else if( dIs( "keyPass" ) || dIs( "pass" ) || dIs( "password" ) )
		{
		  	http_clean_password( h );
			
			if( !valIs( "null" ) && !valIs( "-" ) )
			{
				h->ssl->password = str_copy( av->vptr, av->vlen );
				h->ssl->passlen  = (uint16_t) av->vlen;
			}
		}
		else if( dIs( "enable" ) )
		{
			h->ssl->enabled = config_bool( av );

			if( h->ssl->enabled )
				debug( "SSL enabled on http server." );
		}
		else if( dIs( "port" ) )
		{
			h->ssl->port = (uint16_t) strtoul( av->vptr, NULL, 10 );
			debug( "SSL port set to %hu.", h->ssl->port );
		}
	}
	else
		return -1;

	return 0;
}

#undef dIs

int http_ask_password( void )
{
	static struct termios oldt, newt;
	HTTP_CTL *h = _http;
	volatile char *p;
	size_t bsz;
	int l;

	// can we even ask?
	if( !isatty( STDIN_FILENO ) )
	{
		err( "Cannot get a password - stdin is not a tty." );
		return 1;
	}

	// clear out any existing
	http_clean_password( h );
	h->ssl->password = (volatile char *) allocz( MAX_SSL_PASS_SIZE );

	printf( "Enter SSL key password: " );
	fflush( stdout );

	// save terminal settings
	tcgetattr( STDIN_FILENO, &oldt );
	newt = oldt;

	// unset echoing
	newt.c_lflag &= ~(ECHO);

	// set terminal attributes
	tcsetattr( STDIN_FILENO, TCSANOW, &newt );

	// go get it - buffer is already zerod
	bsz = MAX_SSL_PASS_SIZE;
	l = (int) getline( (char **) &(h->ssl->password), &bsz, stdin );

	// and fix the terminal
	tcsetattr( STDIN_FILENO, TCSANOW, &oldt );

	// and give us a fresh new line
	printf( "\n" );
	fflush( stdout );

	debug( "Password retrieved from command line." );

	// stomp on any newline and carriage return
	// and get the length
	p = h->ssl->password + l - 1;
	while( l > 0 && ( *p == '\n' || *p == '\r' ) )
	{
		l--;
		*p-- = '\0';
	}
	h->ssl->passlen = l;

	return 0;
}


