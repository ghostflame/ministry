/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* http/utils.c - server helper functions                                  *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"


void http_server_panic( void *cls, const char *file, unsigned int line, const char *reason )
{
	herr( "[HTTP] Server panic: (%s/%u) %s", file, line, reason );
}



void *http_log_request( void *cls, const char *uri, HTTP_CONN *conn )
{
	union MHD_ConnectionInfo *ci;

	// see who we are talking to
	ci = (union MHD_ConnectionInfo *) MHD_get_connection_info( conn, MHD_CONNECTION_INFO_CLIENT_ADDRESS );

	hinfo( "[HTTP] Request {%s}: %s", inet_ntoa( ((struct sockaddr_in *) (ci->client_addr))->sin_addr ), uri );

	return req;
}


void http_log( void *arg, const char *fm, va_list ap )
{
	char lbuf[4096];

	vsnprintf( lbuf, 4096, fm, ap );

	hinfo( "[HTTP] %s", lbuf );
}



int __http_cmp_handlers( const void *h1, const void *h2 )
{
	HTPATH *p1 = *((HTPATH **) h1);
	HTPATH *p2 = *((HTPATH **) h2);

	return ( p1->plen < p2->plen ) ? -1 : ( p1->plen == p2->plen ) ? 0 : 1;
}




// set a reporter function after an http call
void http_set_reporter( http_reporter *fp, void *arg )
{
	_http->rpt_fp  = fp;
	_http->rpt_arg = arg;
}


int http_tls_enabled( void )
{
	return _http->tls->enabled;
}

