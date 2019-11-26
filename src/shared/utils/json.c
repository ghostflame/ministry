/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* utils/json.c - utilities around JSON parsing                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"


JSON *parse_json_file( FILE *fh, char *path )
{
	enum json_tokener_error jerr;
	struct json_object *jo;
	int d, cfh = 0;
	char *m;
	off_t s;

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

		cfh = 1;
	}

	rewind( fh );
	d = fileno( fh );

	s = lseek( d, 0, SEEK_END );
	lseek( d, 0, SEEK_SET );

	if( s > _proc->max_json_sz )
	{
		err( "Chickening out of parsing file, size %ld is > than max allowed %d.",
			s, _proc->max_json_sz );
		return NULL;
	}

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

	if( cfh )
		fclose( fh );

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

		json_insert( o, ( ( ok ) ? "result" : "error" ), string, mbuf );
	}

	json_insert( o, "success", boolean, ok );
	strbuf_json( buf, o, 1 );
}

