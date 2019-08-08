/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
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
	if( b->len )
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
		if( ( l + b->len ) > b->sz )
			len = b->sz - b->len - 1;
		else
			len = l;

		if( !len )
			break;

		memcpy( b->buf + b->len, p, len );
		b->len += len;
		b->buf[b->len] = '\0';

		p += len;
		l -= len;

		data_parse_buf( h, b );
	}

	//debug( "There were %d bytes left over after the callback.", b->len );
	return 0;
}



int post_handler( HTREQ *req )
{
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
	for( i = 0; i < DATA_TYPE_MAX; i++ )
	{
		d = data_type_defns + i;
		snprintf( buf1, 24, "/submit/%s", d->name );
		snprintf( buf2, 24, "Submit %s data", d->name );

		// TODO - submission control lists
		http_add_handler( buf1, buf2, d, HTTP_METH_POST, &post_handler, NULL, 0 );
	}

	return 0;
}

