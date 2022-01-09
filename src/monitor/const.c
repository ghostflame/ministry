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
* const.c - constants associated with the monitors                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "min-monitor.h"


const char *monitor_type_names[MON_TYPE_MAX] =
{
	"http",
	"tcp",
	"file"
};

/*

*/

const char *monitor_error_strings[MON_ERR_MAX] =
{
	"no error",
	"connection timeout",
	"connection refused",
	"request failed",
	"request denied",
	"invalid path"
};


int monitor_type_lookup( const char *name )
{
	int i;

	for( i = 0; i < MON_TYPE_MAX; ++i )
		if( !strcasecmp( name, monitor_type_names[i] ) )
			return i;

	return -1;
}

const char *monitor_type_name( int mtype )
{
	if( mtype < 0 || mtype >= MON_TYPE_MAX )
		return "unknown";

	return monitor_type_names[mtype];
}

const char *monitor_error_label( int etype )
{
	if( etype < 0 || etype >= MON_ERR_MAX )
		return "unknown";

	return monitor_error_strings[etype];
}



