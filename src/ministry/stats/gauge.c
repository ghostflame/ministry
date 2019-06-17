/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
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

#ifdef DEBUG
	//debug( "[%02d] Gauge claim", t->id );
#endif

	st_thr_time( steal );

	// take the data
	for( i = 0; i < t->conf->hsize; i++ )
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
					d->empty++;
		}

#ifdef DEBUG
	//debug( "[%02d] Gauge report", t->id );
#endif

	st_thr_time( stats );

	// and report it
	for( i = 0; i < t->conf->hsize; i++ )
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

					t->active++;
				}
			}

	// keep track of all points
	t->total += t->points;

	// how long did all that take?
	st_thr_time( done );
}


