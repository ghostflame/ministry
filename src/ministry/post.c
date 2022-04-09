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
* post.c - functions to handle HTTP posts                                 *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"


int post_handle_init( HTREQ *req )
{
	HOST *h = mem_new_host( &(req->sin), NET_BUF_SZ );

	h->type = ((DTYPE *) req->path->arg)->nt;
	req->post->obj = h;

	if( net_set_host_parser( h, 0, 1 ) )
	{
		warn( "Could not select host parsers for POST callback." );
		return -1;
	}

	return 0;
}


int post_handle_finish( HTREQ *req )
{
	HOST *h = (HOST *) req->post->obj;
	IOBUF *b;

	if( !h )
	{
		debug( "There was no host! (finish)" );
		return -1;
	}

	b = h->net->in;

	// this happens most times, due to the
	// hwmk check in post_handle_data
	if( b->bf->len )
		data_parse_buf( h, b );

	// then free up the host
	mem_free_host( (HOST **) &(req->post->obj) );

	strbuf_printf( req->text, "Received %ld bytes.", req->post->total );
	return 0;
}


int post_handle_data( HTREQ *req )
{
	HOST *h = (HOST *) req->post->obj;
	IOBUF *b = h->net->in;
	const char *p = req->post->data;
	int len, l = req->post->bytes;

	if( !h )
		return -1;

	//debug( "Saw %d new bytes.", l );

	while( l > 0 )
	{
		if( buf_hasspace( b->bf, l ) )
			len = l;
		else
			len = buf_space( b->bf );

		if( !len )
			break;

		buf_appends( b->bf, p, len );
		buf_terminate( b->bf );

		p += len;
		l -= len;

		data_parse_buf( h, b );
	}

	//debug( "There were %d bytes left over after the callback.", b->len );
	return 0;
}





int post_handler( HTREQ *req )
{
	int ret = 0;

	if( req->is_json )
	{
		//notice( "Received a submission as json." );

		// we won't get called until the json is parsed
		// and called just a single time
		ret = data_parse_json( req->post->jo, (DTYPE *) req->path->arg );

		if( ret < 0 )
			req->code = MHD_HTTP_UNPROCESSABLE_CONTENT;

		return ret;
	}

	switch( req->post->state )
	{
		case HTTP_POST_START:
			return post_handle_init( req );
		case HTTP_POST_BODY:
			return post_handle_data( req );
		case HTTP_POST_END:
			return post_handle_finish( req );
	}

	return -1;
}



int post_init( void )
{
	char buf1[24], buf2[24];
	DTYPE *d;
	int i;

	// create a post target for each type
	for( i = 0; i < DATA_TYPE_MAX; ++i )
	{
		d = data_type_defns + i;
		snprintf( buf1, 24, "/submit/%s", d->name );
		snprintf( buf2, 24, "Submit %s data", d->name );

		// TODO - submission control lists
		http_add_handler( buf1, buf2, d, HTTP_METH_POST, &post_handler, NULL, 0 );
	}

	return 0;
}

