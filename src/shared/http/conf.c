/**************************************************************************
* Copyright 2015 John Denholm                                             *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
*                                                                         *
*                                                                         *
* http/config.c - config and setup for libmicrohttpd                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"


HTTP_CTL *_http = NULL;


void http_set_default_ports( uint16_t hport, uint16_t tport )
{
	if( hport )
		_http->port = hport;

	if( tport )
		_http->tls->port = tport;
}



HTTP_CTL *http_config_defaults( void )
{
	struct sockaddr_in *sin;
	HTTP_CTL *h;

	h                  = (HTTP_CTL *) mem_perm( sizeof( HTTP_CTL ) );
	h->tls             = (TLS_CONF *) mem_perm( sizeof( TLS_CONF ) );
	h->calls           = (HTTP_CB *)  mem_perm( sizeof( HTTP_CB ) );

	h->conns_max       = DEFAULT_HTTP_CONN_LIMIT;
	h->conns_max_ip    = DEFAULT_HTTP_CONN_IP_LIMIT;
	h->conns_tmout     = DEFAULT_HTTP_CONN_TMOUT;
	h->post_max        = DEFAULT_POST_MAX_SZ;
	h->port            = DEFAULT_HTTP_PORT;
	h->realm           = str_copy( "ministry", 0 );
	h->addr            = NULL;
	// MHD_USE_DEBUG does *weird* things.  Points *con_cls at the request buffer
	// without it, *con_cls seems to relate to the length of the requested url
	// either way, whiskey tango foxtrot libmicrohttpd
	h->flags           = MHD_USE_THREAD_PER_CONNECTION\
						|MHD_USE_INTERNAL_POLLING_THREAD\
						|MHD_USE_AUTO\
						|MHD_USE_TCP_FASTOPEN\
						|MHD_USE_ERROR_LOG;

	h->enabled         = 0;
	h->ctl_enabled     = 0;
	h->tls->enabled    = 0;
	h->tls->port       = DEFAULT_HTTPS_PORT;
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
	sin = (struct sockaddr_in *) mem_perm( sizeof( struct sockaddr_in ) );
	sin->sin_family      = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;
	h->sin               = sin;

	// and handler structures
	h->get_h          = (HTHDLS *) mem_perm( sizeof( HTHDLS ) );
	h->get_h->method  = "GET";
	h->post_h         = (HTHDLS *) mem_perm( sizeof( HTHDLS ) );
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
			h->addr = av_copy( av );

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

			h->web_iplist = av_copy( av );
			debug( "Using HTTP IP filter %s.", h->web_iplist );
		}
		else if( attIs( "controlFilter" ) )
		{
			if( h->ctl_iplist )
				free( h->ctl_iplist );

			h->ctl_iplist = av_copy( av );
			debug( "Using HTTP controls IP filter %s.", h->ctl_iplist );
		}
		else if( attIs( "postMax" ) )
		{
			if( av_int( v ) == NUM_INVALID )
			{
				err( "Invalid maximum post size '%s'.", av->vptr );
				return -1;
			}

			// we allow 4k - 16M
			if( v < 0x1000 || v > 0x1000000 )
			{
				warn( "Post max size must be between 4k and 16M - resetting to %ld.",
					DEFAULT_POST_MAX_SZ );
				v = DEFAULT_POST_MAX_SZ;
			}

			h->post_max = (size_t) v;
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
	else if( !strncasecmp( av->aptr, "auth.", 5 ) )
	{
		av->alen -= 5;
		av->aptr += 5;

		if( attIs( "enable" ) )
		{
			if( config_bool( av ) )
			{
				flagf_add( h, HTTP_FLAGS_AUTH );
				notice( "Auth is enabled." );
			}
			else
			{
				flagf_rmv( h, HTTP_FLAGS_AUTH );
				notice( "Auth is disabled." );
			}
		}
		else if( attIs( "require" ) || attIs( "enforce" ) )
		{
			if( config_bool( av ) )
			{
				flagf_add( h, HTTP_FLAGS_AUTH_REQ );
				notice( "Auth, if enabled, is required - no anonymous access." );
			}
			else
			{
				flagf_rmv( h, HTTP_FLAGS_AUTH_REQ );
				notice( "Auth, if enabled, is optional - anonymous access allowed." );
			}
		}
		else if( attIs( "realm" ) )
		{
			free( h->realm );
			h->realm = av_copy( av );
			notice( "Auth realm set to '%s'.", h->realm );
		}
		else if( attIs( "source" ) )
		{
			free( h->source );
			h->source = av_copy( av );
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
			h->tls->cert.path = av_copy( av );
		}
		else if( attIs( "keyFile" ) || attIs( "key" ) )
		{
			if( h->tls->key.path )
				free( h->tls->key.path );
			h->tls->key.path = av_copy( av );
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
				h->tls->password = av_copy( av );
				h->tls->passlen  = (uint16_t) av->vlen;
			}
		}
		else if( attIs( "priorities" ) )
		{
			free( h->tls->priorities );
			h->tls->priorities = av_copy( av );
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



