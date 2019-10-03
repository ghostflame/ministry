/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* stats/histo.c - histogram stats functions                               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


void stats_histo_one( ST_THR *t, DHASH *d )
{
	ST_HIST *c = d->proc.hist.conf;
	DHIST *h = &(d->proc.hist);
	int i;


	// iterate through all but one of the bounds, reporting
	// the bounding value B, ie Val <= B
	// but leaving off the +Inf bound
	for( i = 0; i < c->brange; ++i )
	{
		bprintf( t, "%s.%d.bound%s %f",   d->base, d->tags, i, c->bounds[i] );
		bprintf( t, "%s.%d.count%s %lld", d->base, d->tags, i, h->counts[i] );
	}
	// upper bound is +Inf, but we can't easily send that to carbon-cache
	// without it spitting that back as 'Infinity' which is invalid JSON
	// so we send it separately
	// so we can't just set it as the last of the bounds
	bprintf( t, "%s.inf.count%s %lld", d->base, d->tags, h->counts[c->brange] );

	// number of points
	bprintf( t, "%s.total%s %lld", d->base, d->tags, d->proc.count );

	t->points += d->proc.count;

	// keep the highest point count
	if( d->proc.count > t->highest )
		t->highest = d->proc.count;

	// keep track of active paths
	++(t->active);
}



void stats_histo_pass( ST_THR *t )
{
	uint64_t i, sz;
	DHASH *d;

#ifdef DEBUG
	//debug( "[%02d] Histogram claim", t->id );
#endif

	st_thr_time( steal );

	// take the data
	for( i = 0; i < t->conf->hsize; ++i )
		if( ( i % t->max ) == t->id )
		{
			for( d = t->conf->data[i]; d && d->valid; d = d->next )
				if( d->in.count > 0 )
				{
					sz = d->in.hist.conf->bcount * sizeof( int64_t );
					lock_histo( d );

					// copy everything, then zero the in counters
					d->proc.count = d->in.count;
					d->in.count   = 0;
					memcpy( d->proc.hist.counts, d->in.hist.counts, sz );
					memset( d->in.hist.counts, 0, sz );
					d->do_pass  = 1;

					unlock_histo( d );
				}
				else if( d->empty >= 0 )
					++(d->empty);
		}

	st_thr_time( wait );

#ifdef DEBUG
	//debug( "[%02d] Histogram report", t->id );
#endif

	st_thr_time( stats );

	// and report it
	for( i = 0; i < t->conf->hsize; ++i )
		if( ( i % t->max ) == t->id )
			for( d = t->conf->data[i]; d && d->valid; d = d->next )
				if( d->do_pass && d->proc.count > 0 )
				{
					// capture this? - needs a post-report capture mechanism
					// lock, do all the steals, then unlock?
					// spread the cond_signal across all steals?
					if( d->empty > 0 )
						d->empty = 0;

					stats_histo_one( t, d );

					// keep count and then zero it
					t->points += d->proc.count;
					d->proc.count = 0;

					// and remove the pass marker
					d->do_pass = 0;

					++(t->active);
				}

	// keep track of all points
	t->total += t->points;

	// and work out how long that took
	st_thr_time( done );
}


