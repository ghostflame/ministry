/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* http/http.c - handle an http request                                    *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"



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




