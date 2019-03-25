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
	req->post->obj = mem_new_host( req->sin, MIN_NETBUF_SZ );
	req->text = strbuf_resize( req->text, 64 );

	return 0;
}


int post_handle_finish( HTREQ *req )
{
	// bleat about any unprocessed data?
	mem_free_host( (HOST **) &(req->post->obj) );
	return 0;
}


int post_handle_data( HTREQ *req )
{
	DTYPE *dt = (DTYPE *) req->path->arg;

	strbuf_printf( req->text, "Received %ld bytes.", req->post->total );

	notice( "Saw %ld bytes of %s post.", req->post->bytes , dt->name );
	return 0;
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

		http_add_handler( buf1, buf2, d, HTTP_METH_POST, &post_handle_data, &post_handle_init, &post_handle_finish );
	}

	return 0;
}

