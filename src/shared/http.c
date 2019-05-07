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



// called by multiple places, including potentially as policy callback
int http_access_policy( void *cls, const struct sockaddr *addr, socklen_t addrlen )
{
	IPLIST *srcs = (IPLIST *) cls;
	struct in_addr ina;
	IPNET *n = NULL;

	ina = ((struct sockaddr_in *) addr)->sin_addr;

	if( iplist_test_ip( srcs, ina.s_addr, &n ) == IPLIST_NEGATIVE )
		return MHD_NO;

	return MHD_YES;
}


HTPATH *http_find_callback( const char *url, int rlen, HTHDLS *hd )
{
	HTPATH *p;

	for( p = hd->list; p; p = p->next )
		if( p->plen == rlen && !memcmp( p->path, url, rlen ) )
			return p;

	return NULL;
}


int __http_add_handler( char *path, char *desc, void *arg, int method, http_callback *fp, IPLIST *srcs, int flags )
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
	p->flags = flags;
	p->list  = hd;
	p->srcs  = srcs;
	p->cb    = fp;

	p->next  = hd->list;
	hd->list = p;

	hd->count++;
	_http->hdlr_count++;

	// re-sort that list
	mem_sort_list( (void **) &(hd->list), hd->count, __http_cmp_handlers );
	debug( "Added %s%s handler: (%d/%hu) %s  ->  %s", hd->method,
	        ( p->flags & HTTP_FLAGS_CONTROL ) ? " (CONTROL)" : "",
	        hd->count, _http->hdlr_count, p->path, p->desc );

	return 0;
}


int http_add_handler( char *path, char *desc, void *arg, int method, http_callback *fp, IPLIST *srcs, int flags )
{
	return __http_add_handler( path, desc, arg, method, fp, srcs, flags );
}

int http_add_control( char *path, char *desc, void *arg, http_callback *fp, IPLIST *srcs, int flags )
{
	char urlbuf[1024];

	if( *path == '/' )
		path++;

	snprintf( urlbuf, 1024, "/control/%s", path );

	return __http_add_handler( urlbuf, desc, arg, HTTP_METH_POST, fp, srcs, flags|HTTP_FLAGS_CONTROL );
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



int __http_cmp_handlers( const void *h1, const void *h2 )
{
	HTPATH *p1 = *((HTPATH **) h1);
	HTPATH *p2 = *((HTPATH **) h2);

	return ( p1->plen < p2->plen ) ? -1 : ( p1->plen == p2->plen ) ? 0 : 1;
}



int http_send_response( HTREQ *req )
{
	int ret = MHD_YES;
	HTTP_RESP *resp;

	if( req->sent )
	{
		notice( "http_send_response called twice." );
		return ret;
	}

	if( req->text->len > 0 )
	{
		resp = MHD_create_response_from_buffer( req->text->len, (void *) req->text->buf, MHD_RESPMEM_MUST_COPY );
		strbuf_empty( req->text );
	}
	else
	{
		resp = MHD_create_response_from_buffer( 0, "", MHD_RESPMEM_MUST_COPY );
	}

	// is it a json method?
	if( req->path && req->path->flags & HTTP_FLAGS_JSON )
		MHD_add_response_header( resp, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json; charset=utf-8" );

	ret = MHD_queue_response( req->conn, req->code, resp );
	MHD_destroy_response( resp );

	req->sent = 1;

	return ret;
}




void http_request_complete( void *cls, HTTP_CONN *conn,
	void **arg, HTTP_CODE toe )
{
	int64_t nsec, bytes;
	HTREQ *req;

	if( !( req = (HTREQ *) *arg ) )
		return;

	//debug( "Complete with req arg value as %p.", req );

	bytes = ( req->text ) ? req->text->len : 0;

	if( req->sent == 0 )
		http_send_response( req );

	if( req->post )
	{
		req->post->state = HTTP_POST_END;

		if( req->path->flags & HTTP_FLAGS_CONTROL )
			http_calls_ctl_done( req );
		else
			(req->path->cb)( req );

		// free anything on the post object
		if( req->post->objFree )
		{
			free( req->post->objFree );
			req->post->objFree = NULL;
		}
	}

	// call a reporter function if we have one
	if( _http->rpt_fp && !( req->path->flags & HTTP_FLAGS_NO_REPORT ) )
	{
		nsec = get_time64( ) - req->start;
		(_http->rpt_fp)( req->path, _http->rpt_arg, nsec, bytes );
	}

	mem_free_request( &req );
	*arg = NULL;
}


int http_request_access_check( IPLIST *src, HTREQ *req, struct sockaddr *sa )
{
	if( !src )
		return 1;

	if( http_access_policy( src, sa, 0 ) != MHD_YES )
	{
		req->text = strbuf_copy( req->text, "Access denied.", 0 );
		req->code = MHD_HTTP_FORBIDDEN;
		http_send_response( req );
		return 0;
	}

	return 1;
}


HTREQ *http_request_creator( HTTP_CONN *conn, const char *url, const char *method )
{
	union MHD_ConnectionInfo *ci;
	int rlen, is_ctl;
	HTHDLS *hd;
	HTREQ *req;

	req = mem_new_request( );
	req->conn = conn;
	req->code = MHD_HTTP_OK;
	req->first = 1;
	req->start = get_time64( );

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
		req->text = strbuf_copy( req->text, "Method not supported.", 0 );
		req->code = MHD_HTTP_METHOD_NOT_ALLOWED;
		http_send_response( req );
		return NULL;
	}

	// find the handler
	if( !( req->path = http_find_callback( url, rlen, hd ) ) )
	{
		req->text = strbuf_copy( req->text, "That url is not recognised.", 0 );
		req->code = MHD_HTTP_NOT_FOUND;
		http_send_response( req );
		return NULL;
	}

	// is this a control call?
	is_ctl = req->path->flags & HTTP_FLAGS_CONTROL;

	ci = (union MHD_ConnectionInfo *) MHD_get_connection_info( conn, MHD_CONNECTION_INFO_CLIENT_ADDRESS );
	req->sin = *((struct sockaddr_in *) (ci->client_addr));

	// do we have a separate ip list?
	// check it's not the generic one
	// see the commend below
	if( req->path->srcs != _http->web_ips
	 && http_request_access_check( req->path->srcs, req, ci->client_addr ) )
		return NULL;

	hit_counter( req->path->hits );

	if( req->meth == HTTP_METH_POST )
	{
		if( req->first )
		{
			// check this IP is allowed
			if( is_ctl )
			{
				// do we have a controls ip check list?
				// check it's not the generic one
				//
				// note, we've now done this twice, once checking
				// against the generic list, once against the
				// ctls list.  If a path passes the generic list
				// in, we've already checked at access policy time,
				// so we don't check again
				//
				// if the path passed the ctls list in, then we
				// already checked against the controls one on the
				// per-path check above - as that would not have
				// equalled the generic list (unless it's the same
				// list everywhere).
				if( req->path->srcs != _http->ctl_ips
				 && !http_request_access_check( _http->ctl_ips, req, ci->client_addr ) )
					return NULL;
			}

			// and create the post data object
			if( !req->post )
				req->post = (HTTP_POST *) allocz( sizeof( HTTP_POST ) );

			req->post->state = HTTP_POST_START;

			if( is_ctl )
				http_calls_ctl_init( req );
			else
				(req->path->cb)( req );
		}
	}

	return req;
}




int http_request_handler( void *cls, HTTP_CONN *conn, const char *url,
	const char *method, const char *version, const char *upload_data,
	size_t *upload_data_size, void **con_cls )
{
	HTREQ *req;

	// WHISKEY TANGO FOXTROT, libmicrohttpd?
	if( ((int64_t) *con_cls) < 0x1fff )
	{
		//info( "Flattening bizarre arg value %p -> %p.", *con_cls );
		*con_cls = NULL;
	}

	// what the hell is it putting in con_cls, that's supposed to be // null?
	if( ( req = *((HTREQ **) con_cls) ) && req->check != HTTP_CLS_CHECK )
	{
		//info( "Flattening weird con_cls %p -> %p.", con_cls, req );
		*con_cls = NULL;
	}

	// do we have an object?
	if( !( req = *((HTREQ **) con_cls) ) )
	{
		req = http_request_creator( conn, url, method );

		if( req )
			*con_cls = req;
		else
			return MHD_YES;
	}

	switch( req->meth )
	{
		case HTTP_METH_GET:
			// gets are easy
			(req->path->cb)( req );
			http_send_response( req );
			return MHD_YES;

		case HTTP_METH_POST:
			// post arrive as
			// 1.    first call, no data
			// 2...  N data calls
			// 3.    last call, no data
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
				req->post->state = HTTP_POST_BODY;

				if( req->pproc )
				{
					if( MHD_post_process( req->pproc, upload_data, *upload_data_size ) != MHD_YES )
						req->err = 1;
				}
				else
				{
					if( (req->path->cb)( req ) < 0 )
						req->err = 1;
				}
			}
			else
			{
				http_send_response( req );
			}

			// say we have processed the data
			*upload_data_size = 0;

			return MHD_YES;
	}

	warn( "How did the request logic get here?" );
	return MHD_NO;
}





int http_tls_load_file( TLS_FILE *sf, char *type )
{
	char desc[32];

	sf->type = str_dup( type, 0 );

	snprintf( desc, 32, "TLS %s file", sf->type );

	sf->len = MAX_TLS_FILE_SIZE;

	return read_file( sf->path, &(sf->content), &(sf->len), 1, desc );
}



int http_tls_setup( TLS_CONF *s )
{
	if( http_tls_load_file( &(s->cert), "cert" )
	 || http_tls_load_file( &(s->key),  "key"  ) )
		return -1;

	return 0;
}



// stomp on our password/length
void http_clean_password( HTTP_CTL *h )
{
	if( h->tls->password )
	{
		if( h->tls->passlen )
			memset( (void *) h->tls->password, 0, h->tls->passlen );

		free( (void *) h->tls->password );
	}

	h->tls->password = NULL;
	h->tls->passlen = 0;
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

	// are we doing overall connect restriction?
	if( !h->web_ips )
		h->calls->policy = NULL;

	if( h->tls->enabled )
	{
		if( http_tls_setup( h->tls ) )
		{
			err( "Failed to load TLS certificates." );
			http_clean_password( h );
			return -1;
		}

		port = h->tls->port;
		h->proto = "https";
		flag_add( h->flags, MHD_USE_TLS );
	}
	else
	{
		port = h->port;
		h->proto = "http";
	}

	h->sin->sin_port = htons( port );

	MHD_set_panic_func( h->calls->panic, (void *) h );

	if( h->tls->enabled )
	{
		h->server = MHD_start_daemon( h->flags, 0,
			h->calls->policy,                (void *) h->web_ips,
			h->calls->handler,               (void *) h,
			mop( SOCK_ADDR ),                (struct sockaddr *) h->sin,
			mop( URI_LOG_CALLBACK ),         h->calls->reqlog, NULL,
			mop( EXTERNAL_LOGGER ),          h->calls->log, (void *) h,
			mop( NOTIFY_COMPLETED ),         h->calls->complete, NULL,
			mop( CONNECTION_LIMIT ),         h->conns_max,
			mop( PER_IP_CONNECTION_LIMIT ),  h->conns_max_ip,
			mop( CONNECTION_TIMEOUT ),       h->conns_tmout,
			mop( HTTPS_MEM_KEY ),            (const char *) h->tls->key.content,
			mop( HTTPS_MEM_CERT ),           (const char *) h->tls->cert.content,
#if MIN_MHD_PASS > 0
			mop( HTTPS_KEY_PASSWORD ),       (const char *) h->tls->password,
#endif
			mop( HTTPS_PRIORITIES ),         (const char *) h->tls->priorities,
			mop( END )
		);
	}
	else
	{
		h->server = MHD_start_daemon( h->flags, 0,
			h->calls->policy,                (void *) h->web_ips,
			h->calls->handler,               (void *) h,
			mop( SOCK_ADDR ),                (struct sockaddr *) h->sin,
			mop( URI_LOG_CALLBACK ),         h->calls->reqlog, NULL,
			mop( EXTERNAL_LOGGER ),          h->calls->log, (void *) h,
			mop( NOTIFY_COMPLETED ),         h->calls->complete, NULL,
			mop( CONNECTION_LIMIT ),         h->conns_max,
			mop( PER_IP_CONNECTION_LIMIT ),  h->conns_max_ip,
			mop( CONNECTION_TIMEOUT ),       h->conns_tmout,
			mop( END )
		);
	}

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
		pthread_mutex_destroy( &(h->hitlock) );
		notice( "Shut down %s server.", h->proto );
	}
}


// set a reporter function after an http call
void http_set_reporter( http_reporter *fp, void *arg )
{
	_http->rpt_fp  = fp;
	_http->rpt_arg = arg;
}



HTTP_CTL *http_config_defaults( void )
{
	struct sockaddr_in *sin;
	HTTP_CTL *h;

	h                  = (HTTP_CTL *) allocz( sizeof( HTTP_CTL ) );
	h->tls             = (TLS_CONF *) allocz( sizeof( TLS_CONF ) );
	h->calls           = (HTTP_CB *) allocz( sizeof( HTTP_CB ) );

	h->conns_max       = DEFAULT_HTTP_CONN_LIMIT;
	h->conns_max_ip    = DEFAULT_HTTP_CONN_IP_LIMIT;
	h->conns_tmout     = DEFAULT_HTTP_CONN_TMOUT;
	h->port            = DEFAULT_HTTP_PORT;
	h->addr            = NULL;
	h->stats           = 1;
	// MHD_USE_DEBUG does *weird* things.  Points *con_cls at the request buffer
	// without it, *con_cls seems to relate to the length of the requested url
	// either way, whiskey tango foxtrot libmicrohttpd
	h->flags           = MHD_USE_THREAD_PER_CONNECTION|MHD_USE_INTERNAL_POLLING_THREAD|MHD_USE_AUTO|MHD_USE_TCP_FASTOPEN|MHD_USE_ERROR_LOG;

	h->enabled         = 0;
	h->tls->enabled    = 0;

	// only allow 1.3, 1.2 by default
	h->tls->priorities = str_copy( "SECURE256:!VERS-TLS1.1:!VERS-TLS1.0:!VERS-SSL3.0:%SAFE_RENEGOTIATION", 0 );

	h->calls->handler  = &http_request_handler;
	h->calls->policy   = &http_access_policy;
	h->calls->panic    = &http_server_panic;
	h->calls->reader   = &http_unused_reader;
	h->calls->complete = &http_request_complete;
	h->calls->rfree    = &http_unused_reader_free;

	h->calls->log      = &http_log;
	h->calls->reqlog   = &http_log_request;

	// lock for hits counters
	pthread_mutex_init( &(h->hitlock), NULL );

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

	return _http;
}



int http_config_line( AVP *av )
{
	HTTP_CTL *h = _http;

	if( !memchr( av->aptr, '.', av->alen ) )
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

	// then its conns. or tls.

	if( !strncasecmp( av->aptr, "conns.", 6 ) )
	{
		av->alen -= 6;
		av->aptr += 6;

		if( attIs( "max" ) )
		{
			h->conns_max = (unsigned int) strtoul( av->vptr, NULL, 10 );
		}
		else if( attIs( "maxPerIp" ) || attIs( "maxPerHost" ) )
		{
			h->conns_max_ip = (unsigned int) strtoul( av->vptr, NULL, 10 );
		}
		else if( attIs( "timeout" ) || attIs( "tmout" ) )
		{
			h->conns_tmout = (unsigned int) strtoul( av->vptr, NULL, 10 );
		}
		else
			return -1;
	}
	else if( !strncasecmp( av->aptr, "tls.", 4 ) )
	{
		av->alen -= 4;
		av->aptr += 4;

		if( attIs( "certFile" ) || attIs( "cert" ) )
		{
			if( h->tls->cert.path )
				free( h->tls->cert.path );
			h->tls->cert.path = str_copy( av->vptr, av->vlen );
		}
		else if( attIs( "keyFile" ) || attIs( "key" ) )
		{
			if( h->tls->key.path )
				free( h->tls->key.path );
			h->tls->key.path = str_copy( av->vptr, av->vlen );
		}
		else if( attIs( "keyPass" ) || attIs( "pass" ) || attIs( "password" ) )
		{
		  	http_clean_password( h );
			
			if( !valIs( "null" ) && !valIs( "-" ) )
			{
				h->tls->password = str_copy( av->vptr, av->vlen );
				h->tls->passlen  = (uint16_t) av->vlen;
			}
		}
		else if( attIs( "priorities" ) )
		{
			free( h->tls->priorities );
			h->tls->priorities = str_copy( av->vptr, av->vlen );
		}
		else if( attIs( "enable" ) )
		{
			h->tls->enabled = config_bool( av );

			if( h->tls->enabled )
				debug( "TLS enabled on http server." );
		}
		else if( attIs( "port" ) )
		{
			h->tls->port = (uint16_t) strtoul( av->vptr, NULL, 10 );
			debug( "HTTPS port set to %hu.", h->tls->port );
		}
	}
	else
		return -1;

	return 0;
}



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
	h->tls->password = (volatile char *) allocz( MAX_TLS_PASS_SIZE );

	printf( "Enter TLS key password: " );
	fflush( stdout );

	// save terminal settings
	tcgetattr( STDIN_FILENO, &oldt );
	newt = oldt;

	// unset echoing
	newt.c_lflag &= ~(ECHO);

	// set terminal attributes
	tcsetattr( STDIN_FILENO, TCSANOW, &newt );

	// go get it - buffer is already zerod
	bsz = MAX_TLS_PASS_SIZE;
	l = (int) getline( (char **) &(h->tls->password), &bsz, stdin );

	// and fix the terminal
	tcsetattr( STDIN_FILENO, TCSANOW, &oldt );

	// and give us a fresh new line
	printf( "\n" );
	fflush( stdout );

	debug( "Password retrieved from command line." );

	// stomp on any newline and carriage return
	// and get the length
	p = h->tls->password + l - 1;
	while( l > 0 && ( *p == '\n' || *p == '\r' ) )
	{
		l--;
		*p-- = '\0';
	}
	h->tls->passlen = l;

	return 0;
}


