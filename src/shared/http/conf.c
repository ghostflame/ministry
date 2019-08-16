/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* http/config.c - config and setup for libmicrohttpd                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"


HTTP_CTL *_http = NULL;




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
	// MHD_USE_DEBUG does *weird* things.  Points *con_cls at the request buffer
	// without it, *con_cls seems to relate to the length of the requested url
	// either way, whiskey tango foxtrot libmicrohttpd
	h->flags           = MHD_USE_THREAD_PER_CONNECTION|MHD_USE_INTERNAL_POLLING_THREAD|MHD_USE_AUTO|MHD_USE_TCP_FASTOPEN|MHD_USE_ERROR_LOG;

	h->enabled         = 0;
	h->ctl_enabled     = 0;
	h->tls->enabled    = 0;
	h->ctl_iplist      = str_copy( "localhost-only", 0 );

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

	// and handler structures
	h->get_h          = (HTHDLS *) allocz( sizeof( HTHDLS ) );
	h->get_h->method  = "GET";
	h->post_h         = (HTHDLS *) allocz( sizeof( HTHDLS ) );
	h->post_h->method = "POST";

	_http = h;

	return _http;
}



int http_config_line( AVP *av )
{
	HTTP_CTL *h = _http;
	int64_t v;

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
			debug( "Http server is %sabled.", BOOL_ENABLED( h->enabled ) );
		}
		else if( attIs( "controls" ) )
		{
			h->ctl_enabled = config_bool( av );
			debug( "Http controls are %sabled.", BOOL_ENABLED( h->ctl_enabled ) );
		}
		else if( attIs( "httpFilter" ) )
		{
			if( h->web_iplist )
				free( h->web_iplist );

			h->web_iplist = str_copy( av->vptr, av->vlen );
			debug( "Using HTTP IP filter %s.", h->web_iplist );
		}
		else if( attIs( "controlFilter" ) )
		{
			if( h->ctl_iplist )
				free( h->ctl_iplist );

			h->ctl_iplist = str_copy( av->vptr, av->vlen );
			debug( "Using HTTP controls IP filter %s.", h->ctl_iplist );
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
			if( av_int( v ) == NUM_INVALID )
			{
				err( "Invalid max connections '%s'", av->vptr );
				return -1;
			}
			h->conns_max = (unsigned int) v;
		}
		else if( attIs( "maxPerIp" ) || attIs( "maxPerHost" ) )
		{
			if( av_int( v ) == NUM_INVALID )
			{
				err( "Invalid max-per-ip connections '%s'", av->vptr );
				return -1;
			}
			h->conns_max_ip = (unsigned int) v;
		}
		else if( attIs( "timeout" ) || attIs( "tmout" ) )
		{
			if( av_int( v ) == NUM_INVALID )
			{
				err( "Invalid connection timeout '%s'", av->vptr );
				return -1;
			}
			h->conns_tmout = (unsigned int) v;
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

			if( valIs( "-" ) )
			{
				debug( "Asking for TLS key password interactively - config said '-'." );
				setcfFlag( KEY_PASSWORD );
				return 0;
			}

			if( !valIs( "null" ) )
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



