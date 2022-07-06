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
* stats/gauge.c - gauge type functions                                    *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



void stats_gauge_pass( ST_THR *t )
{
	uint64_t i;
	DHASH *d;

	st_thr_time( steal );

	// take the data
	for( i = 0; i < t->conf->hsize; ++i )
		if( ( i % t->max ) == t->id )
		{
			for( d = t->conf->data[i]; d && d->valid; d = d->next )
				if( d->in.count )
				{
					lock_gauge( d );

					d->proc.count = d->in.count;
					d->proc.total = d->in.total;
					// don't reset the gauge, just the count
					d->in.count = 0;

					unlock_gauge( d );
				}
				else if( d->empty >= 0 )
					++(d->empty);
		}

	st_thr_time( stats );

	// and report it
	for( i = 0; i < t->conf->hsize; ++i )
		if( ( i % t->max ) == t->id )
			for( d = t->conf->data[i]; d; d = d->next )
			{
				// we report gauges anyway, updated or not
				bprintf( t, "%s %f", d->path, d->proc.total );

				if( d->proc.count )
				{
					if( d->empty > 0 )
						d->empty = 0;

					// keep count and zero the counter
					t->points += d->proc.count;
					d->proc.count = 0;

					++(t->active);
				}
			}

	// keep track of all points
	t->total += t->points;

	// how long did all that take?
	st_thr_time( done );
}


