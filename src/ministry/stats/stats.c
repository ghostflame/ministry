/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* stats/stats.c - statistics functions and config                         *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"




// report on standard deviation, skewness and kurtosis
void stats_report_moments( ST_THR *t, DHASH *d, int64_t ct, double mean )
{
	MOMS m;

	memset( &m, 0, sizeof( MOMS ) );
	m.input    = t->wkspc;
	m.count    = ct;
	m.mean     = mean;
	m.mean_set = 1;

	maths_moments( &m );

	bprintf( t, "%s.stddev %f",   d->path, m.sdev );
	bprintf( t, "%s.skewness %f", d->path, m.skew );
	bprintf( t, "%s.kurtosis %f", d->path, m.kurt );
}


void stats_report_mode( ST_THR *t, DHASH *d, int64_t ct )
{
	int64_t i, mdct, mdmx;
	double mode, mtmp;

	mdmx = 0;
	mdct = 0;

	mtmp = t->wkspc[0] - 1;

	for( i = 0; i < ct; i++ )
	{
		if( t->wkspc[i] == mtmp )
			mdct++;
		else
		{
			if( mdct > mdmx )
			{
				mdmx = mdct;
				mode = mtmp;
			}
			mdct = 0;
			mtmp = t->wkspc[i];
		}
	}
	if( mdct > mdmx )
	{
		mdmx = mdct;
		mode = mtmp;
	}

	if( mdmx > 1 )
	{
		bprintf( t, "%s.mode %f",    d->path, mode );
		bprintf( t, "%s.mode_ct %f", d->path, mdmx );
	}
}

void __report_array( double *arr, int64_t ct )
{
	char abuf[8192];
	int x, y, i, j;

	i = ( ct > 30 ) ? 15 : ct / 2;
	j = ( ct > 30 ) ? ( ct - 15 ) : ct / 2;

	for( y = 0, x = 0; x < i; x++ )
		y += snprintf( abuf + y, 8192 - y, " %.1f", arr[x] );

	y += snprintf( abuf + y, 8192 - y, "   .....   " );

	for( x = j; x < ct; x++ )
		y += snprintf( abuf + y, 8192 - y, " %.1f", arr[x] );

	debug( "Array (%ld): %s", ct, abuf + 1 );
}


void stats_report_one( ST_THR *t, DHASH *d )
{
	int64_t i, ct, idx;
	double sum, mean;
	PTLIST *list, *p;
	ST_THOLD *thr;

	// grab the points list
	list = d->proc.points;
	d->proc.points = NULL;

#ifdef CATCH_HIGH_POINTERS
	// weird pointer corruption check
	if( ((unsigned long) list) & 0xfff0000000000000 )
	{
		fatal( "Found ptr val %p on dhash %s",
			list, d->path );
		return;
	}
#endif

	// anything to do?
	if( ( ct = (int32_t) d->proc.count ) == 0 )
	{
		if( list )
			mem_free_point_list( list );
		return;
	}

	// if we have just one points structure, we just use
	// it's own vals array as our workspace.  We need to
	// sort in place, but only this fn holds that space
	// now, so that's ok.  If we have more than one, we
	// need a flat array, so make sure the workbuf is big
	// enough and copy each vals array into it

	if( list->next == NULL )
		t->wkspc = list->vals;
	else
	{
		// do we have enough workspace?
		stats_set_workspace( t, ct );

		// and the workspace is the buffer
		t->wkspc = t->wkbuf1;

		// now copy the data in
		for( i = 0, p = list; p; p = p->next )
		{
			memcpy( t->wkspc + i, p->vals, p->count * sizeof( double ) );
			i += p->count;
		}
	}

	sum = 0;
	maths_kahan_summation( t->wkspc, ct, &sum );

	// median offset
	idx = ct / 2;

	// and the mean
	mean = sum / (double) ct;

	// and sort them
	if( ct < ctl->stats->qsort_thresh )
		sort_qsort_dbl( t, (int32_t) ct );
	else
		sort_radix11( t, (int32_t) ct );

	bprintf( t, "%s.count %d",  d->path, ct );
	bprintf( t, "%s.mean %f",   d->path, mean );
	bprintf( t, "%s.upper %f",  d->path, t->wkspc[ct-1] );
	bprintf( t, "%s.lower %f",  d->path, t->wkspc[0] );
	bprintf( t, "%s.median %f", d->path, t->wkspc[idx] );

	// variable thresholds
	for( thr = ctl->stats->thresholds; thr; thr = thr->next )
	{
		// find the right index into our values
		idx = ( thr->val * ct ) / thr->max;
		bprintf( t, "%s.%s %f", d->path, thr->label, t->wkspc[idx] );
	}

	// are we doing std deviation and friends?
	if( dhash_do_moments( d ) && ctl->stats->mom->min_pts <= ct )
		stats_report_moments( t, d, ct, mean );

	// Do we want the mode?
	if( dhash_do_mode( d ) && ct >= ctl->stats->mode->min_pts )
		stats_report_mode( t, d, ct );

	mem_free_point_list( list );

	// keep count
	t->points += ct;

	// and keep highest
	if( ct > t->highest )
		t->highest = ct;

	// and keep track of active
	t->active++;
}


#define st_thr_time( _name )		clock_gettime( CLOCK_REALTIME, &(t->_name) )


void stats_stats_pass( ST_THR *t )
{
	uint64_t i;
	PTLIST *p;
	DHASH *d;

#ifdef DEBUG
	//debug( "[%02d] Stats claim", t->id );
#endif

	st_thr_time( steal );

	// take the data
	for( i = 0; i < t->conf->hsize; i++ )
		if( ( i % t->max ) == t->id )
		{
			for( d = t->conf->data[i]; d && d->valid; d = d->next )
				if( d->valid && d->in.count )
				{
					// prefetch a points object
					// outside the lock
					// this may fix some of the
					// locking issues under high load
					p = mem_new_point( );

					lock_stats( d );

					d->proc.points = d->in.points;
					d->proc.count  = d->in.count;
					d->in.points   = p;
					d->in.count    = 0;
					d->do_pass     = 1;

					unlock_stats( d );
				}
				else if( d->empty >= 0 )
					d->empty++;
		}

#ifdef DEBUG
	//debug( "[%02d] Stats report", t->id );
#endif

	st_thr_time( stats );

	// and report it
	for( i = 0; i < t->conf->hsize; i++ )
		if( ( i % t->max ) == t->id )
			for( d = t->conf->data[i]; d && d->valid; d = d->next )
				if( d->do_pass && d->proc.points )
				{
					if( d->empty > 0 )
						d->empty = 0;

					stats_report_one( t, d );

					d->do_pass = 0;
				}

	// keep track of all points
	t->total += t->points;

	// and work out how long that took
	st_thr_time( done );
}



