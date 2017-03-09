/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* http.c - invokes microhttpd for serving to prometheus                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "ministry.h"



// URL HANDLERS



void http_handler_disallowed( RESP *r )
{
	strbuf_copy( r->buf, "Only GET and POST are supported.\r\n", 0 );
	r->code = MHD_HTTP_METHOD_NOT_ALLOWED;
}

void http_handler_unknown( RESP *r )
{
	strbuf_copy( r->buf, "That url is not recognised.\r\n", 0 );
	r->code = MHD_HTTP_NOT_FOUND;
}


void http_handler_tokens( RESP *r )
{
	int types, i, count;
	TOKEN *t, *tlist[8];

	types = TOKEN_TYPE_STATS|TOKEN_TYPE_ADDER|TOKEN_TYPE_GAUGE;
	token_generate( r->peer.sin_addr.s_addr, types, tlist, 8, &count );

	strbuf_copy( r->buf, "{", 1 );

	// run down the tokens
	for( i = 0; i < count; i++ )
	{
		t = tlist[i];
		strbuf_aprintf( r->buf, "\"%s\": %ld,", t->name, t->nonce );
	}

	// chop off the trailing ,
	// hand-crafting json is such a pain but all the C libs to
	// do it really, really suck.
	if( strbuf_lastchar( r->buf ) == ',' )
		strbuf_chop( r->buf );

	strbuf_add( r->buf, "}\r\n", 1 );
}


void http_handler_usage( RESP *r )
{
	HTTP_CTL *h = ctl->http;
	int i;

	for( i = URL_ID_SLASH; i < URL_ID_MAX; i++ )
		strbuf_aprintf( r->buf, "%-16s %s\r\n", h->map[i].url, h->map[i].desc );
}


void http_handler_status( RESP *r )
{
	strbuf_copy( r->buf, "Ministry internal stats.\r\n", 0 );
}



int http_copy_lines( IOBUF *b, const char *ptr, size_t len )
{
	const char *p;
	size_t l, max;

	max = b->sz - b->len - 2;

	if( len < max )
	{
		memcpy( b->buf + b->len, ptr, len );
		b->len += len;
	}
	else if( ( p = memrchr( ptr, '\n', max ) ) )
	{
		p++;
		l = p - ptr;
		memcpy( b->buf + b->len, ptr, l );
		b->len += l;
		len = l;
	}
	else
	{
		// we need to add a newline
		memcpy( b->buf + b->len, ptr, max );
		b->len += max;
		b->buf[b->len++] = '\n';
		len = max;
	}

	b->buf[b->len] = '\0';
	return (int) len;
}


void http_handle_buffer( RESP *r, NET_TYPE *ntype )
{
	const char *ptr;
	int len, l;
	HOST *h;

	if( net_ip_check( &(r->peer) ) != 0 )
	{
		notice( "Denying HTTP submission from %s:%hu based on ip check.",
			inet_ntoa( r->peer.sin_addr ), ntohs( r->peer.sin_port ) );

		r->code = MHD_HTTP_FORBIDDEN;
		strbuf_copy( r->buf, "This IP is not permitted.\r\n", 0 );
		return;
	}

	if( !( h = r->host ) )
	{
		if( !( h = mem_new_host( &(r->peer), MIN_NETBUF_SZ ) ) )
		{
			err( "Could not allocate new host." );
			r->code = MHD_HTTP_INTERNAL_SERVER_ERROR;
			return;
		}

		h->type = ntype;

		if( net_set_host_parser( h, 1, 1 ) )
		{
			err( "Could not set host parser." );
			r->code = MHD_HTTP_INTERNAL_SERVER_ERROR;
			mem_free_host( &h );
			return;
		}

		r->host = h;
	}

	ptr = r->data;
	len = r->dlen;

	while( len > 0 )
	{
		l = http_copy_lines( h->net->in, ptr, len );
		len -= l;
		ptr += l;
		info( "Copied %d bytes, %d remaining.", l, len );

		data_parse_buf( h, h->net->in );
	}

	info( "[HTTP] Handler %lu points to %s", h->points, r->url->url );
	strbuf_printf( r->buf, "You submitted %lu points to %s\r\n",
		h->points, r->url->url );
}



void http_handler_data_stats( RESP *r )
{
	http_handle_buffer( r, ctl->net->stats );
}

void http_handler_data_adder( RESP *r )
{
	http_handle_buffer( r, ctl->net->adder );
}

void http_handler_data_gauge( RESP *r )
{
	http_handle_buffer( r, ctl->net->gauge );
}

void http_handler_data_compat( RESP *r )
{
	http_handle_buffer( r, ctl->net->compat );
}




// OTHER FNS


int http_unused_policy( void *cls, const struct sockaddr *addr, socklen_t addrlen )
{
	// replace with IP host check?
	notice( "Connection from %s", inet_ntoa( ((const struct sockaddr_in *) addr)->sin_addr ) );
	return MHD_YES;
}

ssize_t http_unused_reader( void *cls, uint64_t pos, char *buf, size_t max )
{
	notice( "Unused reader." );
	return -1;
}

void http_unused_reader_free( void *cls )
{
	notice( "Unused reader free." );
	return;
}

int http_unused_kv( void *cls, HTTP_VAL kind, const char *key, const char *value )
{
	notice( "Unused KV" );
	return MHD_NO;
}

int http_post_iterator( void *cls, HTTP_VAL kind, const char *key, const char *filename,
                    const char *content_type, const char *transfer_encoding, const char *data,
                    uint64_t off, size_t size )
{
	notice( "Unused POST: %ld", (long) size );
	return MHD_YES;
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






int http_do_response( HTTP_CONN *conn, RESP *r )
{
	HTTP_RESP *resp;
	int ret;

	resp = MHD_create_response_from_buffer( r->buf->len, (void *) r->buf->buf, MHD_RESPMEM_MUST_COPY );
	info( "Resp: %p", resp );
	ret  = MHD_queue_response( conn, r->code, resp );
	info( "Ret: (%d) %d", r->code, ret );
	MHD_destroy_response( resp );

	mem_free_resp( &r );

	if( ret == MHD_YES )
		info( "Returning successful." );
	else
		info( "Returning failure." );

	return ret;
}


static int http_request_id = 0;

int http_request_handler( void *cls, HTTP_CONN *conn, const char *url,
	const char *method, const char *version, const char *up_data,
	size_t *up_size, void **req )
{
	union MHD_ConnectionInfo *cinfo;
	struct sockaddr_in *sin;
	HTTP_CTL *h = ctl->http;
	int rlen, j;
	RESP *r;
	URL *u;

	notice( "HTTP Request Handler called: %s %p", method, *req );

	// first call?
	// for some reason, req is showing up with 0x47/0x48 (G/H) in it
	// wtf
	if( !( ((long unsigned int) *req) & 0xffff0000 ) )
	{
		r = mem_new_resp( h->max_resp );
		r->code = MHD_HTTP_OK;
		r->url  = h->disallowed;
		r->id   = ++http_request_id;

		// see who we are talking to
		cinfo = (union MHD_ConnectionInfo *) MHD_get_connection_info( conn, MHD_CONNECTION_INFO_CLIENT_ADDRESS );
		sin   = (struct sockaddr_in *) (cinfo->client_addr);

		r->peer.sin_addr.s_addr = sin->sin_addr.s_addr;
		r->peer.sin_port        = sin->sin_port;

		// work out the method
		if( !strcasecmp( method, MHD_HTTP_METHOD_POST ) )
			r->meth = METHOD_POST;
		else if( !strcasecmp( method, MHD_HTTP_METHOD_GET ) )
			r->meth = METHOD_GET;
		else
			r->meth = METHOD_UNKNOWN;

		// we only support post and get for now
		if( r->meth == METHOD_POST || r->meth == METHOD_GET )
		{
			// switch to 404
			r->url = h->unknown;

			// go find the url
			rlen = strlen( url );
			for( u = h->map, j = URL_ID_SLASH; j < URL_ID_MAX; j++, u++ )
				if( rlen == u->len && !memcmp( url, u->url, rlen ) )
				{
					r->url = u;
					//debug( "Url: %s", u->url );
					break;
				}
		}

		r->post = MHD_create_post_processor( conn, 65536, h->calls->post, r );

		// and use it as the closure
		info( "Setting *req for request %d", r->id );
		*req = r;

		// posts are done here
		if( r->meth == METHOD_POST )
			return MHD_YES;
	}
	else
		r = *((RESP **) req);


	if( r->meth == METHOD_POST )
	{
		if( *up_size > 0 )
		{
			MHD_post_process( r->post, up_data, *up_size );
			*up_size = 0;
		}
		else
		{
			info( "Destroying post processor." );
			MHD_destroy_post_processor( r->post );
			return http_do_response( conn, r );
		}
	}
	else
	{
		(*(r->url->fp))( r );
	}

	return MHD_YES;
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

	// do things here

	return 0;
}




#define mop( _o )		MHD_OPTION_ ## _o

int http_start( void )
{
	HTTP_CTL *h = ctl->http;
	uint16_t port;

	if( !h->enabled )
		return 0;

	if( h->ssl->enabled )
	{
		if( http_ssl_setup( h->ssl ) )
			return -1;

		port = h->ssl->port;
		h->proto = "https";
		h->flags |= MHD_USE_SSL;
	}
	else
	{
		port = h->port;
		h->proto = "http";
	}

	h->sin->sin_port = htons( port );

	MHD_set_panic_func( h->calls->panic, (void *) h );

	h->server = MHD_start_daemon( h->flags, 0,
			h->calls->policy, NULL,
			h->calls->handler, NULL,
			mop( CONNECTION_LIMIT ),         h->conns_max,
			mop( PER_IP_CONNECTION_LIMIT ),  h->conns_max_ip,
			mop( CONNECTION_TIMEOUT ),       h->conns_tmout,
//			mop( NOTIFY_COMPLETED ),         h->calls->complete, NULL,
			mop( SOCK_ADDR ),                (struct sockaddr *) h->sin,
			mop( URI_LOG_CALLBACK ),         h->calls->reqlog, NULL,
			mop( HTTPS_MEM_KEY ),            (const char *) h->ssl->key.content,
			mop( HTTPS_KEY_PASSWORD ),       (const char *) h->ssl->password,
			mop( HTTPS_MEM_CERT ),           (const char *) h->ssl->cert.content,
			mop( EXTERNAL_LOGGER ),          h->calls->log, (void *) h,
			mop( END )
		);

	if( !h->server )
	{
		err( "Failed to start %s server.", h->proto );
		return -1;
	}

	notice( "Started up %s server on port %hu.", h->proto, port );
	return 0;
}

#undef mop

void http_stop( void )
{
	HTTP_CTL *h = ctl->http;
	if( h->server )
	{
		MHD_stop_daemon( h->server );
		notice( "Shut down %s server.", h->proto );
	}
}


URL url_mapping[URL_ID_MAX] = {
	// these
	{ URL_ID_DISALLOWED, &http_handler_disallowed,  0, 0,  "",                "Disallowed"                     },
	{ URL_ID_UNKNOWN,    &http_handler_unknown,     0, 0,  "",                "404 Handler"                    },
	// should come before this
	{ URL_ID_SLASH,      &http_handler_usage,       0, 0,  "/",               "View URL Map"                   },
	{ URL_ID_STATUS,     &http_handler_status,      0, 0,  "/status",         "Ministry internal status"       },
	{ URL_ID_TOKENS,     &http_handler_tokens,      0, 0,  "/tokens",         "Connection tokens endpoint"     },
	{ URL_ID_STATS,      &http_handler_data_stats,  1, 0,  "/stats",          "Stats submission endpoint"      },
	{ URL_ID_ADDER,      &http_handler_data_adder,  1, 0,  "/adder",          "Adder submission endpoint"      },
	{ URL_ID_GAUGE,      &http_handler_data_gauge,  1, 0,  "/gauge",          "Gauge submission endpoint"      },
	{ URL_ID_COMPAT,     &http_handler_data_compat, 1, 0,  "/compat",         "Statsd submission endpoint"     }
//	{ URL_ID_METRICS,    &http_handler_metrics,     0, 0,  "/metrics",        "Prometheus metrics endpoint"    }
};


HTTP_CTL *http_config_defaults( void )
{
	struct sockaddr_in *sin;
	HTTP_CTL *h;
	int i;

	h                  = (HTTP_CTL *) allocz( sizeof( HTTP_CTL ) );
	h->ssl             = (SSL_CONF *) allocz( sizeof( SSL_CONF ) );
	h->calls           = (HTTP_CB *) allocz( sizeof( HTTP_CB ) );

	h->conns_max       = DEFAULT_HTTP_CONN_LIMIT;
	h->conns_max_ip    = DEFAULT_HTTP_CONN_IP_LIMIT;
	h->conns_tmout     = DEFAULT_HTTP_CONN_TMOUT;
	h->port            = DEFAULT_HTTP_PORT;
	h->addr            = NULL;
	h->status          = 1;
	h->flags           = MHD_USE_THREAD_PER_CONNECTION|MHD_USE_POLL|MHD_USE_DEBUG;
	h->max_resp        = DEFAULT_HTTP_MAX_RESP;

	h->map             = url_mapping;

	h->enabled         = 0;
	h->ssl->enabled    = 0;

	h->calls->handler  = &http_request_handler;
	h->calls->panic    = &http_server_panic;
	h->calls->policy   = &http_unused_policy;
	h->calls->reader   = &http_unused_reader;
	h->calls->rfree    = &http_unused_reader_free;

	h->calls->kv       = &http_unused_kv;
	h->calls->post     = &http_post_iterator;

	h->calls->log      = &http_log;
	h->calls->reqlog   = &http_log_request;


	// make the address binding structure
	sin = (struct sockaddr_in *) allocz( sizeof( struct sockaddr_in ) );
	sin->sin_family      = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;
	h->sin               = sin;

	// fix the lengths on the url mappings
	// and pick some out
	for( i = 0; i < URL_ID_MAX; i++ )
		switch( h->map[i].id )
		{
			case URL_ID_DISALLOWED:
				h->disallowed = h->map + i;
				break;
			case URL_ID_UNKNOWN:
				h->unknown = h->map + i;
				break;
			default:
				h->map[i].len = strlen( h->map[i].url );
		}

	return h;
}


#define dIs( _str )		!strcasecmp( d, _str )

int http_config_line( AVP *av )
{
	HTTP_CTL *h = ctl->http;
	int64_t v;
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
		else if( attIs( "status" ) || attIs( "exposeStatus" ) )
		{
			h->status = config_bool( av );
			debug( "Http exposure of internal status is %sabled.",
				( h->status ) ? "en" : "dis" );
		}
		else if( attIs( "maxResponse" ) )
		{
			if( parse_number( av->val, &v, NULL ) || v > INT32_MAX )
			{
				err( "Invalid http server max response size '%s'.", av->val );
				return -1;
			}
			h->max_resp = (int32_t) v;
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

