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
* query/config.c - query config functions                                 *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


QRY_CTL *_qry = NULL;


void query_init( void )
{
	//http_add_handler( char *path, char *desc, void *arg, int method, http_callback *fp, IPLIST *srcs, int flags )

	http_add_handler( "/query", "Query the timeseries store", NULL, HTTP_METH_GET,
		&query_get_callback, NULL, HTTP_FLAGS_JSON );
}


QRY_CTL *query_config_defaults( void )
{
	_qry = (QRY_CTL *) allocz( sizeof( QRY_CTL ) );

	_qry->default_timespan = DEFAULT_QUERY_TIMESPAN * MILLION;
	_qry->max_paths = DEFAULT_QUERY_MAX_PATHS;

	return _qry;
}


int query_config_line( AVP *av )
{
	int64_t v;

	if( attIs( "defaultSpan" ) )
	{
		if( time_span_usec( av->vptr, &v ) != 0 )
		{
			err( "Invalid default query span: %s", av->vptr );
			return -1;
		}

		_qry->default_timespan = v;
	}
	else if( attIs( "maxPaths" ) )
	{
		if( av_int( v ) == NUM_INVALID )
		{
			err( "Invalid max query paths: %s", av->vptr );
			return -1;
		}

		_qry->max_paths = v;
	}
	else
		return -1;

	return 0;
}


