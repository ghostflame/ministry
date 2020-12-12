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
* utils/json.c - utilities around JSON parsing                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"


JSON *parse_json_file( FILE *fh, const char *path )
{
	struct json_object *jo = NULL;
	enum json_tokener_error jerr;
	struct json_tokener *jtk;
	int d = -1, cfh = 0;
	struct stat sb;
	void *m;

	if( fh )
	{
		rewind( fh );
		d = fileno( fh );
	}
	else
	{
		if( !path )
		{
			err( "No path to parse for JSON." );
			return NULL;
		}

		d = open( path, O_RDONLY );
		if( d < 0 )
		{
			err( "Cannot open path '%s' to parse for JSON -- %s",
				path, Err );
			return NULL;
		}

		cfh = 1;
	}

	if( fstat( d, &sb ) != 0 )
	{
		err( "Could not stat JSON file %s -- %s", path, Err );
		goto JSON_FILE_DONE;
	}

	if( sb.st_size > _proc->max_json_sz )
	{
		err( "Chickening out of parsing file, size %ld is > than max allowed %d.",
			sb.st_size, _proc->max_json_sz );
		goto JSON_FILE_DONE;
	}

	m = mmap( NULL, sb.st_size, PROT_READ, MAP_PRIVATE, d, 0 );

	if( m == MAP_FAILED )
	{
		err( "Could not mmap file for JSON parsing -- %s", Err );
		goto JSON_FILE_DONE;
	}

	// don't need this any more
	if( cfh )
		close( d );
	d = -1;

	jtk  = json_tokener_new( );
	jo   = json_tokener_parse_ex( jtk, (char *) m, sb.st_size );
	jerr = json_tokener_get_error( jtk );

	json_tokener_free( jtk );

	munmap( m, sb.st_size );

	if( jerr != json_tokener_success )
	{
		err( "Could not parse JSON file -- %s",
			json_tokener_error_desc( jerr ) );

		if( jo )
		{
			json_object_put( jo );
			jo = NULL;
		}
	}

JSON_FILE_DONE:
	if( cfh && d >= 0 )
		close( d );

	return jo;
}


void create_json_result( BUF *buf, int ok, const char *fmt, ... )
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

