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
* data/point.c - individual type point handlers                           *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"



__attribute__((hot)) void data_point_histo( const char *path, int len, const char *dat )
{
	double v;
	DHASH *d;

	v = strtod( dat, NULL );
	d = data_get_dhash( path, len, ctl->stats->histo );

	data_update_histo( d, v, '\0' );
}



// this has no associated update function - json won't send us
__attribute__((hot)) void data_point_gauge( const char *path, int len, const char *dat )
{
	double v;
	DHASH *d;
	char op;

	// gauges can have relative changes
	if( *dat == '+' || *dat == '-' )
		op = *dat++;
	else
		op = '\0';

	// which means to explicitly set a gauge to a negative
	// number, you must first set it to zero.  Don't blame me,
	// this follows the statsd guide
	// https://github.com/etsy/statsd/blob/master/docs/metric_types.md
	v = strtod( dat, NULL );
	d = data_get_dhash( path, len, ctl->stats->gauge );

	data_update_gauge( d, v, op );
}




__attribute__((hot)) void data_point_adder( const char *path, int len, const char *dat )
{
	double val;
	DHASH *d;

	val = strtod( dat, NULL );

	d = data_get_dhash( path, len, ctl->stats->adder );

	// lock that path
	lock_adder( d );

	// add in that data point
	d->in.total += val;
	++(d->in.count);

	// and unlock
	unlock_adder( d );
}


__attribute__((hot)) void data_point_stats( const char *path, int len, const char *dat )
{
	double v;
	DHASH *d;

	v = strtod( dat, NULL );
	d = data_get_dhash( path, len, ctl->stats->stats );

	data_update_stats( d, v, '\0' );
}

