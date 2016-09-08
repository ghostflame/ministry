/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* http.c - invokes microhttpd for serving to prometheus                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "ministry.h"






int *http_request_handler( void *cls, HTTP_CONN *conn,
	const char *url, const char *method, const char *version,
	const char *upload_data, size_t upload_data_size,
	void **con_cls )
{

	// we only support GET right now
	if( strcasecmp( method, "GET" ) )
		return MHD_NO;




	return MHD_YES;
}


void *http_request_complete( void *cls, HTTP_CONN *conn,
	void **con_cls, HTTP_CODE toe )
{

	return NULL;
}







int http_start( void )
{
	CTL_HTTP *h = ctl->http;

	if( ! h->enabled )
		return 0;


	return 0;
}





HTTP_CTL *http_default_config( void )
{
	HTTP_CTL *h;

	h = (HTTP_CTL *) allocz( sizeof( HTTP_CTL ) );

	h->port    = DEFAULT_HTTP_PORT;
	h->enabled = 0;
	h->stats   = 1;

	return h;
}



int httpd_config_line( APV *av )
{
	HTTP_CTL *h = ctl->http;

	if( attIs( "port" ) )
	{
		h->port = (uint16_t) ( strtoul( av->val, NULL, 10 ) & 0xffffUL );
		debug( "Http port set to %hu.", h->port );
	}
	else if( attIs( "enable" ) )
	{
		h->enabled = config_bool( av );
		debug( "Http server is %sabled.", ( h->enabled ) ? "en" : "dis" );
	}
	else if( attIs( "stats" ) || attIs( "expose-stats" ) )
	{
		h->stats = config_bool( av );
		debug( "Http exposure of internal stats is %sabled.",
			( h->stats ) ? "en" : "dis" );
	}
	else
		return -1;

	return 0;
}


