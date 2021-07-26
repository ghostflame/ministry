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
* data/json.c - handling functions for json data                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"


int data_parse_json_element( DTYPE *dt, json_object *el )
{
	json_object *po, *vo, *ve, *va;
	const char *path, *sval;
	enum json_type jt;
	int plen, is_gg;
	size_t i, vlen;
	double val;
	DHASH *d;
	char op;

	// look for the path
	po = json_object_object_get( el, "path" );
	vo = json_object_object_get( el, "values" );

	if( !po && !vo )
	{
		po = json_object_object_get( el, "target" );
	    vo = json_object_object_get( el, "datapoints" );
	}

	if( !( po && vo )
	 || !json_object_is_type( po, json_type_string )
	 || !json_object_is_type( vo, json_type_array ) )
		return -1;

	path = json_object_get_string( po );
	plen = json_object_get_string_len( po );

	// reject paths with parentheses - they have functions in
	if( memchr( path, '(', plen ) )
		return 1;

	if( !( d = data_get_dhash( (char *) path, plen, dt->stc ) ) )
		return 1;

	vlen = json_object_array_length( vo );

	// no values?  OK... but why?
	if( !vlen )
		return 0;

	// is this a gauge type?  We need to 
	// check the sign and pass that in if so
	is_gg = ( dt->type == DATA_TYPE_GAUGE );

	for( i = 0; i < vlen; i++ )
	{
		if( !( ve = json_object_array_get_idx( vo, i ) ) )
			continue;

		jt = json_object_get_type( ve );
		op = '\0';

		// if you put something other than strings
		switch( jt )
		{
			case json_type_string:
				sval = json_object_get_string( ve );
				if( is_gg )
				{
					if( *sval == '+' || *sval == '-' )
						op = *sval++;
				}

				val = strtod( sval, NULL );
				break;

			case json_type_double:
				val = json_object_get_double( ve );
				break;

			case json_type_array:
				// graphite format
				if( json_object_array_length( ve ) != 2 )
					continue;

				// format is value, timestamp
				// also, null happens
				if( !( va = json_object_array_get_idx( ve, 0 ) ) )
					continue;

				val = json_object_get_double( va );
				break;

			default:
				//info( "No type." );
				continue;
		}

		//info( "[%s] Update %s with value %f, op %c.",
		//	dt->name, d->path, val, op );

		// check val is valid
		if( !isnormal( val ) )
			continue;

		// call the update function
		(*(dt->uf))( d, val, op );
	}

	return 0;
}


/*
 * We expect
[
	{
		"path": "<path1>",
		"values": [ <list of doubles or strings> ]
	},
	{
		"path": "<path2>",
		"values": [ <list of doubles or strings> ]
	},
	...
]
 */

int data_parse_json( json_object *jo, DTYPE *dt )
{
	size_t i, pl, ret = 0;
	json_object *el;

	if( !jo )
	{
		warn( "No json object from json post?!" );
		return -1;
	}

	// we expect an array
	if( !json_object_is_type( jo, json_type_array ) )
		return -1;

	pl = json_object_array_length( jo );

	for( i = 0; i < pl; ++i )
	{
		if( !( el = json_object_array_get_idx( jo, i ) ) )
			continue;

		ret += data_parse_json_element( dt, el );
	}

	return ret;
}

void data_fetch_jcb( void *arg, json_object *jo )
{
	FETCH *f = (FETCH *) arg;
	data_parse_json( jo, f->dtype );
}
