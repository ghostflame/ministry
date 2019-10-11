/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* data/http.c - http interface functions                                  *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"

int data_http_rm_path( DTYPE *dt, const char *path, int len )
{
	DHASH *d;

	if( !( d = data_find_dhash( path, len, dt->stc ) ) )
		return 0;

	debug( "Removing %s path %s", dt->name, path );

	// mark it invalid and gc will get it
	lock_dhash( d );
	d->valid = 0;
	unlock_dhash( d );

	return 1;
}


int data_http_rmpaths( HTREQ *req )
{
	JSON *to, *po, *ro, *rro, *rno, *rrto, *rnto, *rpo;
	int i, sl, rc = 0, nc = 0;
	const char *path;
	size_t j, len;
	DTYPE *dt;

	if( !req->is_json || !req->post->jo )
	{
		req->code = MHD_HTTP_UNPROCESSABLE_ENTITY;
		return -1;
	}

	ro  = json_object_new_object( );
	rro = NULL;
	rno = NULL;

	for( i = 0; i < DATA_TYPE_MAX; ++i )
	{
		dt = data_type_defns + i;
		if( !dt->stc )
			continue;

		if( !( to = json_object_object_get( req->post->jo, dt->name ) ) )
			continue;

		if( json_object_get_type( to ) != json_type_array )
			continue;

		rrto = NULL;
		rnto = NULL;

		len = json_object_array_length( to );

		for( j = 0; j < len; ++j )
		{
			if( !( po = json_object_array_get_idx( to, j ) ) )
				continue;

			if( json_object_get_type( po ) != json_type_string )
				continue;

			if( !( path = json_object_get_string( po ) )
			 || !( sl = json_object_get_string_len( po ) ) )
				continue;

			rpo = json_object_new_string( path );

			if( data_http_rm_path( dt, path, sl ) )
			{
				if( !rrto )
					rrto = json_object_new_array( );

				++rc;
				json_object_array_add( rrto, rpo );
			}
			else
			{
				if( !rnto )
					rnto = json_object_new_array( );

				++nc;
				json_object_array_add( rnto, rpo );
			}
		}

		if( rrto )
		{
			if( !rro )
				rro = json_object_new_object( );

			json_object_object_add( rro, dt->name, rrto );
		}

		if( rnto )
		{
			if( !rno )
				rno = json_object_new_object( );

			json_object_object_add( rno, dt->name, rnto );
		}
	}

	if( rro )
		json_object_object_add( ro, "removed", rro );

	if( rno )
		json_object_object_add( ro, "not-found", rno );

	strbuf_json( req->text, ro, 1 );
	return 0;
}



int data_http_init( void )
{
	int ret = 0;

	// rmpaths requires GC
	if( ctl->gc->enabled )
		ret += http_add_control( "rmpaths", "Remove data paths", NULL, &data_http_rmpaths, NULL, 0 );

	return ret;
}


