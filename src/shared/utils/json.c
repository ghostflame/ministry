/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* utils/json.c - utilities around JSON parsing                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"


json_object *parse_json_file( FILE *fh, char *path )
{
	enum json_tokener_error jerr;
	struct json_object *jo;
	char *m;
	off_t s;
	int d;

	if( !fh )
	{
		if( !path )
		{
			err( "No path to parse for JSON." );
			return NULL;
		}

		if( !( fh = fopen( path, "r" ) ) )
		{
			err( "Cannot open path '%s' to parse for JSON -- %s",
				path, Err );
			return NULL;
		}
	}

	rewind( fh );
	d = fileno( fh );

	s = lseek( d, 0, SEEK_END );
	lseek( d, 0, SEEK_SET );

	m = mmap( NULL, s, PROT_READ, MAP_PRIVATE, d, 0 );

	if( m == MAP_FAILED )
	{
		err( "Could not mmap file for JSON parsing -- %s", Err );
		return NULL;
	}

	jo = json_tokener_parse_verbose( m, &jerr );

	munmap( m, s );

	if( jerr != json_tokener_success )
	{
		if( jo )
			json_object_put( jo );

		return NULL;
	}

	return jo;
}


void create_json_result( BUF *buf, int ok, char *fmt, ... )
{
	json_object *o;

	o = json_object_new_object( );

	if( fmt )
	{
		char mbuf[2048];
		va_list args;

		va_start( args, fmt );
		vsnprintf( mbuf, 2048, fmt, args );
		va_end( args );

		json_object_object_add( o, ( ok ) ? "result" : "error", json_object_new_string( mbuf ) );
	}

	json_object_object_add( o, "success", json_object_new_boolean( ok ) );
	strbuf_json( buf, o, 1 );
}

