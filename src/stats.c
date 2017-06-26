/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* stats.c - statistics functions and config                               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"



const char *stats_type_names[STATS_TYPE_MAX] =
{
	"stats", "adder", "gauge", "self"
};




static inline int cmp_floats( const void *p1, const void *p2 )
{
	double *d1, *d2;

	d1 = (double *) p1;
	d2 = (double *) p2;

	return ( *d1 > *d2 ) ?  1 :
	       ( *d2 < *d1 ) ? -1 : 0;
}



// a little caution so we cannot write off the
// end of the buffer
void bprintf( ST_THR *t, char *fmt, ... )
{
	int i, total;
	va_list args;
	uint32_t l;
	TSET *s;

	// write the variable part into the thread's path buffer
	va_start( args, fmt );
	l = (uint32_t) vsnprintf( t->path->buf, t->path->sz, fmt, args );
	va_end( args );

	// check for truncation
	if( l > t->path->sz )
		l = t->path->sz;

	t->path->len = l;

	// length of prefix and path and max ts buf
	total = t->prefix->len + t->path->len + TSBUF_SZ;

	// loop through the target sets, making sure we can
	// send to them
	for( i = 0; i < ctl->tgt->set_count; i++ )
	{
		s = ctl->tgt->setarr[i];

		// are we ready for a new buffer?
		if( ( t->bp[i]->len + total ) >= IO_BUF_SZ )
		{
			target_buf_send( s, t->bp[i] );

			if( !( t->bp[i] = mem_new_buf( IO_BUF_SZ ) ) )
			{
				fatal( "Could not allocate a new IOBUF." );
				return;
			}
		}

		// so write out the new data
		(*(s->stype->wrfp))( t, t->ts[i], t->bp[i] );
	}
}



// TIMESTAMP FUNCTIONS
void stats_tsf_sec( ST_THR *t, BUF *b )
{
	b->len = snprintf( b->buf, TSBUF_SZ, " %ld\n", t->now.tv_sec );
}

void stats_tsf_msval( ST_THR *t, BUF *b )
{
	b->len = snprintf( b->buf, TSBUF_SZ, " %ld.%03ld\n", t->now.tv_sec, t->now.tv_nsec / 1000000 );
}

void stats_tsf_tval( ST_THR *t, BUF *b )
{
	b->len = snprintf( b->buf, TSBUF_SZ, " %ld.%06ld\n", t->now.tv_sec, t->now.tv_nsec / 1000 );
}

void stats_tsf_tspec( ST_THR *t, BUF *b )
{
	b->len = snprintf( b->buf, TSBUF_SZ, " %ld.%09ld\n", t->now.tv_sec, t->now.tv_nsec );
}

void stats_tsf_msec( ST_THR *t, BUF *b )
{
	b->len = snprintf( b->buf, TSBUF_SZ, " %ld\n", t->tval / 1000000 );
}

void stats_tsf_usec( ST_THR *t, BUF *b )
{
	b->len = snprintf( b->buf, TSBUF_SZ, " %ld\n", t->tval / 1000 );
}

void stats_tsf_nsec( ST_THR *t, BUF *b )
{
	b->len = snprintf( b->buf, TSBUF_SZ, " %ld\n", t->tval );
}




// configure the line buffer of a thread from a prefix config
void stats_set_bufs( ST_THR *t, ST_CFG *c, int64_t tval )
{
	TSET *s;
	int i;

	// only set the timestamp buffer if we need to
	if( tval )
	{
		// grab the timestamp
		t->tval = tval;
		llts( tval, t->now );

		// and reset counters
		t->active  = 0;
		t->points  = 0;
		t->highest = 0;
	}

	// default to our own config
	if( !c )
		c = t->conf;

	// just point to the prefix buffer we want
	t->prefix = c->prefix;

	// grab new buffers
	for( i = 0; i < ctl->tgt->set_count; i++ )
	{
		s = ctl->tgt->setarr[i];

		// only set the timestamp buffers if we need to
		if( tval )
			(*(s->stype->tsfp))( t, t->ts[i] );

		// grab a new buffer
		if( !t->bp[i] && !( t->bp[i] = mem_new_buf( IO_BUF_SZ ) ) )
		{
			fatal( "Could not allocate a new IOBUF." );
			return;
		}
	}
}


// report on a single thread's performance
void stats_thread_report( ST_THR *t )
{
	int64_t p, tsteal, tstats, twait, tdone, delay, steal, wait, stats, total;
	double intvpc;

	// report some self stats?
	if( !ctl->stats->self->enable )
		return;

	stats_set_bufs( t, ctl->stats->self, 0 );

	// we have to capture it, because bprintf increments it
	p = t->active;

	bprintf( t, "%s.active %d", t->wkrstr, t->active );
	bprintf( t, "%s.points %d", t->wkrstr, t->points );

	if( t->conf->type == STATS_TYPE_STATS )
	{
		bprintf( t, "%s.workspace %d", t->wkrstr, t->wkspcsz );
		bprintf( t, "%s.highest %d",   t->wkrstr, t->highest );
	}

	tsteal = tsll( t->steal );
	twait  = tsll( t->wait );
	tstats = tsll( t->stats );
	tdone  = tsll( t->done );

	delay  = tsteal - t->tval;
	stats  = tdone  - tstats;
	total  = tdone  - t->tval;

	if( twait )
	{
		steal = twait  - tsteal;
		wait  = tstats - twait;
	}
	else
	{
		steal = tstats - tsteal;
		wait  = 0;
	}

	bprintf( t, "%s.delay %ld", t->wkrstr, delay / 1000 );
	bprintf( t, "%s.steal %ld", t->wkrstr, steal / 1000 );
	bprintf( t, "%s.stats %ld", t->wkrstr, stats / 1000 );
	bprintf( t, "%s.usec %ld",  t->wkrstr, total / 1000 );

	if( wait )
		bprintf( t, "%s.wait %ld",  t->wkrstr, wait  / 1000 );

	// calculate percentage of interval
	intvpc  = (double) ( total / 10 );
	intvpc /= (double) t->conf->period;
	bprintf( t, "%s.interval_usage %.3f", t->wkrstr, intvpc );

	// and report our own paths
	bprintf( t, "%s.self_paths %ld", t->wkrstr, t->active - p + 1 );
}



// an implementation of Kaham Summation
// https://en.wikipedia.org/wiki/Kahan_summation_algorithm
// useful to avoid floating point errors
static inline void kahan_sum( double val, double *sum, double *low )
{
    double y, t;

    y = val - *low;     // low starts off small
    t = *sum + y;       // sum is big, y small, lo-order y is lost

    *low = ( t - *sum ) - y;// (t-sum) is hi-order y, -y recovers lo-order
    *sum = t;       // low is algebraically always 0
}

void kahan_summation( double *list, int len, double *sum )
{
    double low = 0;
    int i;

    for( *sum = 0, i = 0; i < len; i++ )
        kahan_sum( list[i], sum, &low );

    *sum += low;
}


// https://en.wikipedia.org/wiki/Standard_deviation#Estimation
// https://en.wikipedia.org/wiki/Skewness#Sample_skewness
// https://en.wikipedia.org/wiki/Kurtosis#Sample_kurtosis
void stats_report_moments( ST_THR *t, DHASH *d, int64_t ct, double mean )
{
	double sdev, skew, kurt, dtmp, stmp, ktmp, diff, prod;
	int64_t i;

	sdev = skew = kurt = 0;
	dtmp = stmp = ktmp = 0;

	for( i = 0; i < ct; i++ )
	{
		// diff from mean
		diff = t->wkspc[i] - mean;
		prod = diff * diff;

		// stddev needs sum of squares of diffs
		kahan_sum( prod, &sdev, &dtmp );

		// skewness needs third moment
		prod *= diff;
		kahan_sum( prod, &skew, &stmp );

		// kurtosis needs fourth moment
		prod *= diff;
		kahan_sum( prod, &kurt, &ktmp );
	}

	// complete the kahan sum
	sdev += dtmp;
	skew += stmp;
	kurt += ktmp;

	// we don't need corrected - we have the whole population
	sdev /= (double) ct;
	kurt /= (double) ct;

	// using Fisher-Pearson standardized moment with any decent count size
	// http://www.statisticshowto.com/skewness/
	if( ct > 5 )
	{
		skew *= (double) ct;
		skew /= (double) ( ct - 1 ) * ( ct - 2 );
	}
	else
		skew /= (double) ct;

	// and sqrt the variance to get the std deviation
	sdev = sqrt( sdev );

	// normalize against the variance
	skew /= sdev * sdev * sdev;
	kurt /= sdev * sdev * sdev * sdev;
	// and subtract 3 from kurtosis
	kurt -= 3;

	bprintf( t, "%s.stddev %f",   d->path, sdev );
	bprintf( t, "%s.skewness %f", d->path, skew );
	bprintf( t, "%s.kurtosis %f", d->path, kurt );
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
	if( ( ct = d->proc.count ) == 0 )
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
		if( ct > t->wkspcsz )
		{
			int sz = t->wkspcsz;
			double *ws;

			// double it until we have enough
			while( ct > sz )
				sz *= 2;

			if( !( ws = (double *) allocz( sz * sizeof( double ) ) ) )
			{
				fatal( "Could not allocate new workbuf of %d doubles.", sz );
				return;
			}

			// free the existing and grab a new chunk
			free( t->wkbuf );
			t->wkspcsz = sz;
			t->wkbuf   = ws;
		}

		// and the workspace is the buffer
		t->wkspc = t->wkbuf;

		// now copy the data in
		for( i = 0, p = list; p; p = p->next )
		{
			memcpy( t->wkspc + i, p->vals, p->count * sizeof( double ) );
			i += p->count;
		}
	}

	sum = 0;
	kahan_summation( t->wkspc, ct, &sum );

	// median offset
	idx = ct / 2;

	// and the mean
	mean = sum / (double) ct;

	// and sort them
	qsort( t->wkspc, ct, sizeof( double ), cmp_floats );

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
	if( d->mom_check && ctl->stats->mom->min_pts <= ct )
		stats_report_moments( t, d, ct, mean );

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
	DHASH *d;

#ifdef DEBUG
	debug( "[%02d] Stats claim", t->id );
#endif

	st_thr_time( steal );

	// take the data
	for( i = 0; i < t->conf->hsize; i++ )
		if( ( i % t->max ) == t->id )
		{
			for( d = t->conf->data[i]; d && d->valid; d = d->next )
				if( d->valid && d->in.points )
				{
					lock_stats( d );

					d->proc.points = d->in.points;
					d->proc.count  = d->in.count;
					d->in.points   = NULL;
					d->in.count    = 0;
					d->do_pass     = 1;

					unlock_stats( d );
				}
				else if( d->empty >= 0 )
					d->empty++;
		}

#ifdef DEBUG
	debug( "[%02d] Stats report", t->id );
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

	// and work out how long that took
	st_thr_time( done );
}



void stats_adder_pass( ST_THR *t )
{
	uint64_t i;
	DHASH *d;

#ifdef DEBUG
	debug( "[%02d] Adder claim", t->id );
#endif

	st_thr_time( steal );

	// take the data
	for( i = 0; i < t->conf->hsize; i++ )
		if( ( i % t->max ) == t->id )
		{
			for( d = t->conf->data[i]; d && d->valid; d = d->next )
				if( d->in.count > 0 )
				{
					lock_adder( d );

					// copy everything, then zero the in
					d->proc     = d->in;
					d->in.total = 0;
					d->in.count = 0;
					d->do_pass  = 1;

					unlock_adder( d );
				}
				else if( d->empty >= 0 )
					d->empty++;
		}

	st_thr_time( wait );

	debug_synth( "[%02d] Unlocking adder lock.", t->id );

	// synth thread is waiting for this
	unlock_stthr( t );

	debug_synth( "[%02d] Trying to lock synth.", t->id );

	// try to lock the synth thread
	lock_synth( );

	debug_synth( "[%02d] Unlocking synth.", t->id );

	// and then unlock it
	unlock_synth( );

	debug_synth( "[%02d] Trying to get our own lock back.", t->id );

	// and lock our own again
	lock_stthr( t );

#ifdef DEBUG
	debug( "[%02d] Adder report", t->id );
#endif

	st_thr_time( stats );

	// and report it
	for( i = 0; i < t->conf->hsize; i++ )
		if( ( i % t->max ) == t->id )
			for( d = t->conf->data[i]; d && d->valid; d = d->next )
				if( d->do_pass && d->proc.count > 0 )
				{
					if( d->empty > 0 )
						d->empty = 0;

					bprintf( t, "%s %f", d->path, d->proc.total );

					// keep count and then zero it
					t->points += d->proc.count;
					d->proc.count = 0;

					// and remove the pass marker
					d->do_pass = 0;

					t->active++;
				}


	// and work out how long that took
	st_thr_time( done );
}


void stats_gauge_pass( ST_THR *t )
{
	uint64_t i;
	DHASH *d;

#ifdef DEBUG
	debug( "[%02d] Gauge claim", t->id );
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
	debug( "[%02d] Gauge report", t->id );
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

	// how long did all that take?
	st_thr_time( done );
}


#undef st_thr_time




void thread_pass( int64_t tval, void *arg )
{
	ST_THR *t = (ST_THR *) arg;
	int i;

	// set up bufs and such
	stats_set_bufs( t, NULL, tval );

	// do the work
	(*(t->conf->statfn))( t );

	// and report on ourself, except self-stats
	if( t->conf->type != STATS_TYPE_SELF )
		stats_thread_report( t );

	// send any outstanding data
	for( i = 0; i < ctl->tgt->set_count; i++ )
		if( t->bp[i] )
		{
			target_buf_send( ctl->tgt->setarr[i], t->bp[i] );
			t->bp[i] = NULL;
		}
}



void *stats_loop( void *arg )
{
	ST_CFG *cf;
	ST_THR *c;
	THRD *t;

	t  = (THRD *) arg;
	c  = (ST_THR *) t->arg;
	cf = c->conf;

	// and then loop round
	loop_control( cf->name, thread_pass, c, cf->period, LOOP_SYNC, cf->offset );

	// and unlock ourself
	unlock_stthr( c );

	free( t );
	return NULL;
}



void stats_start( ST_CFG *cf )
{
	int i;

	if( cf->enable == 0 )
	{
		notice( "Data reporting for %s is disabled.", cf->name );
		return;
	}

	// throw each of the threads
	for( i = 0; i < cf->threads; i++ )
		thread_throw( &stats_loop, &(cf->ctls[i]), i );

	info( "Started %s data processing loops.", cf->name );
}




// set a prefix, make sure of a trailing .
// return a copy of a prefix with a trailing .
void stats_prefix( ST_CFG *c, char *s )
{
	int len, dot = 0;

	if( !c->prefix )
		c->prefix = strbuf( PREFIX_SZ );

	len = strlen( s );

	if( len > 0 )
	{
		// do we need a dot?
		if( s[len - 1] != '.' )
			dot = 1;
	}

	if( ( strbuf_copy( c->prefix, s, len ) < 0 )
	 || ( dot && ( strbuf_add(  c->prefix, ".", 1 ) < 0 ) ) )
	{
		fatal( "Prefix is larger than allowed max %d", c->prefix->sz );
		return;
	}
}




void stats_init_control( ST_CFG *c, int alloc_data )
{
	char wkrstrbuf[128];
	int i, l, j;
	ST_THR *t;

	// maybe fall back to default hash size
	if( c->hsize == 0 )
		c->hsize = ctl->mem->hashsize;

	debug( "Hash size set to %d for %s", c->hsize, c->name );

	// create the hash structure
	if( alloc_data )
		c->data = (DHASH **) allocz( c->hsize * sizeof( DHASH * ) );

	// convert msec to usec
	c->period *= 1000;
	c->offset *= 1000;
	// offset can't be bigger than period
	c->offset  = c->offset % c->period;

	// make the control structures
	c->ctls = (ST_THR *) allocz( c->threads * sizeof( ST_THR ) );

	for( i = 0; i < c->threads; i++ )
	{
		t = &(c->ctls[i]);

		t->conf   = c;
		t->id     = i;
		t->max    = c->threads;

		// worker path
		l = snprintf( wkrstrbuf, 128, "workers.%s.%lu", c->name, t->id );
		t->wkrstr = str_dup( wkrstrbuf, l );

		// timestamp buffers
		t->ts = (BUF **) allocz( ctl->tgt->set_count * sizeof( BUF * ) );
		for( j = 0; j < ctl->tgt->set_count; j++ )
			t->ts[j] = strbuf( TSBUF_SZ );

		// and a path workspace
		t->path = strbuf( PATH_SZ );

		// and make space for a buffer for each target set we must write to
		t->bp = (IOBUF **) allocz( ctl->tgt->set_count * sizeof( IOBUF * ) );

		// make some floats workspace - we realloc this if needed
		if( c->type == STATS_TYPE_STATS )
		{
			t->wkspcsz = 1024;
			t->wkbuf   = (double *) allocz( t->wkspcsz * sizeof( double ) );
		}

		pthread_mutex_init( &(t->lock), NULL );

		// and that starts locked
		lock_stthr( t );
	}
}



void stats_init( void )
{
	struct timespec ts;

	stats_init_control( ctl->stats->stats, 1 );
	stats_init_control( ctl->stats->adder, 1 );
	stats_init_control( ctl->stats->gauge, 1 );

	// let's not always seed from 1, eh?
	clock_gettime( CLOCK_REALTIME, &ts );
	srandom( ts.tv_nsec );

	// we only allow one thread for this, and no data
	ctl->stats->self->threads = 1;
	stats_init_control( ctl->stats->self, 0 );
}




STAT_CTL *stats_config_defaults( void )
{
	STAT_CTL *s;

	s = (STAT_CTL *) allocz( sizeof( STAT_CTL ) );

	s->stats          = (ST_CFG *) allocz( sizeof( ST_CFG ) );
	s->stats->threads = DEFAULT_STATS_THREADS;
	s->stats->statfn  = &stats_stats_pass;
	s->stats->period  = DEFAULT_STATS_MSEC;
	s->stats->type    = STATS_TYPE_STATS;
	s->stats->dtype   = DATA_TYPE_STATS;
	s->stats->name    = stats_type_names[STATS_TYPE_STATS];
	s->stats->hsize   = 0;
	s->stats->enable  = 1;
	stats_prefix( s->stats, DEFAULT_STATS_PREFIX );

	s->adder          = (ST_CFG *) allocz( sizeof( ST_CFG ) );
	s->adder->threads = DEFAULT_ADDER_THREADS;
	s->adder->statfn  = &stats_adder_pass;
	s->adder->period  = DEFAULT_STATS_MSEC;
	s->adder->type    = STATS_TYPE_ADDER;
	s->adder->dtype   = DATA_TYPE_ADDER;
	s->adder->name    = stats_type_names[STATS_TYPE_ADDER];
	s->adder->hsize   = 0;
	s->adder->enable  = 1;
	stats_prefix( s->stats, DEFAULT_ADDER_PREFIX );

	s->gauge          = (ST_CFG *) allocz( sizeof( ST_CFG ) );
	s->gauge->threads = DEFAULT_GAUGE_THREADS;
	s->gauge->statfn  = &stats_gauge_pass;
	s->gauge->period  = DEFAULT_STATS_MSEC;
	s->gauge->type    = STATS_TYPE_GAUGE;
	s->gauge->dtype   = DATA_TYPE_GAUGE;
	s->gauge->name    = stats_type_names[STATS_TYPE_GAUGE];
	s->gauge->hsize   = 0;
	s->gauge->enable  = 1;
	stats_prefix( s->stats, DEFAULT_GAUGE_PREFIX );

	s->self           = (ST_CFG *) allocz( sizeof( ST_CFG ) );
	s->self->threads  = 1;
	s->self->statfn   = &self_stats_pass;
	s->self->period   = DEFAULT_STATS_MSEC;
	s->self->type     = STATS_TYPE_SELF;
	s->self->dtype    = -1;
	s->self->name     = stats_type_names[STATS_TYPE_SELF];
	s->self->enable   = 1;
	stats_prefix( s->stats, DEFAULT_SELF_PREFIX );

	// moment checks are off by default
	s->mom            = (ST_MOM *) allocz( sizeof( ST_MOM ) );
	s->mom->min_pts   = DEFAULT_MOM_MIN;
	s->mom->enabled   = 0;
	s->mom->rgx       = regex_list_create( 1 );

	return s;
}

int stats_config_line( AVP *av )
{
	char *d, *pm, *lbl, *fmt, thrbuf[64];
	int i, t, l, mid, top;
	ST_THOLD *th;
	STAT_CTL *s;
	ST_CFG *sc;
	int64_t v;
	WORDS wd;

	s = ctl->stats;

	if( !( d = strchr( av->att, '.' ) ) )
	{
		if( attIs( "thresholds" ) )
		{
			if( strwords( &wd, av->val, av->vlen, ',' ) <= 0 )
			{
				warn( "Invalid thresholds string: %s", av->val );
				return -1;
			}
			if( wd.wc > 20 )
			{
				warn( "A maximum of 20 thresholds is allowed." );
				return -1;
			}

			for( i = 0; i < wd.wc; i++ )
			{
				t = atoi( wd.wd[i] );

				// sort out percent from per-mille
				if( ( pm = memchr( wd.wd[i], 'm', wd.len[i] ) ) )
				{
					mid = 500;
					top = 1000;
					lbl = "per-mille";
					fmt = "%s_%03d";
				}
				else
				{
					mid = 50;
					top = 100;
					lbl = "percent";
					fmt = "%s_%02d";
				}

				// sanity check before we go any further
				if( t <= 0 || t == mid || t >= top )
				{
					warn( "A %s threshold value of %s makes no sense: t != %d, 0 < t < %d.",
						lbl, wd.wd[i], mid, top );
					return -1;
				}

				l = snprintf( thrbuf, 64, fmt, ( ( t < mid ) ? "lower" : "upper" ), t );

				// OK, make a struct
				th = (ST_THOLD *) allocz( sizeof( ST_THOLD ) );
				th->val = t;
				th->max = top;
				th->label = str_dup( thrbuf, l );

				th->next = ctl->stats->thresholds;
				ctl->stats->thresholds = th;
			}

			debug( "Acquired %d thresholds.", wd.wc );
		}
		else if( attIs( "period" ) )
		{
			av_int( v );
			if( v > 0 )
			{
				s->stats->period = v;
				s->adder->period = v;
				s->gauge->period = v;
				s->self->period  = v;
				debug( "All stats periods set to %d msec.", v );
			}
			else
				warn( "Stats period must be > 0, value %d given.", v );
		}
		else
			return -1;

		return 0;
	}

	d++;

	if( !strncasecmp( av->att, "stats.", 6 ) )
		sc = s->stats;
	else if( !strncasecmp( av->att, "adder.", 6 ) )
		sc = s->adder;
	// because I'm nice that way (plus, I keep mis-typing it...)
	else if( !strncasecmp( av->att, "gauge.", 6 ) || !strncasecmp( av->att, "guage.", 6 ) )
		sc = s->gauge;
	else if( !strncasecmp( av->att, "self.", 5 ) )
		sc = s->self;
	else if( !strncasecmp( av->att, "moments.", 8 ) )
	{
		if( !strcasecmp( d, "enable" ) )
		{
			s->mom->enabled = config_bool( av );
		}
		else if( !strcasecmp( d, "minimum" ) )
		{
			av_int( s->mom->min_pts );
		}
		else if( !strcasecmp( d, "fallbackMatch" ) )
		{
			t = config_bool( av );
			regex_list_set_fallback( t, s->mom->rgx );
		}
		else if( !strcasecmp( d, "whitelist" ) )
		{
			if( regex_list_add( av->val, 0, s->mom->rgx ) )
				return -1;
			debug( "Added moments whitelist regex: %s", av->val );
		}
		else if( !strcasecmp( d, "blacklist" ) )
		{
			if( regex_list_add( av->val, 1, s->mom->rgx ) )
				return -1;
			debug( "Added moments blacklist regex: %s", av->val );
		}
		else
			return -1;

		return 0;
	}
	else
		return -1;

	if( !strcasecmp( d, "threads" ) )
	{
		av_int( v );
		if( v > 0 )
			sc->threads = v;
		else
			warn( "Stats threads must be > 0, value %d given.", v );
	}
	else if( !strcasecmp( d, "enable" ) )
	{
		if( !( sc->enable = config_bool( av ) ) )
			debug( "Stats type %s disabled.", sc->name );
	}
	else if( !strcasecmp( d, "prefix" ) )
	{
		stats_prefix( sc, av->val );
		debug( "%s prefix set to '%s'", sc->name, sc->prefix->buf );
	}
	else if( !strcasecmp( d, "period" ) )
	{
		av_int( v );
		if( v > 0 )
			sc->period = v;
		else
			warn( "Stats period must be > 0, value %d given.", v );
	}
	else if( !strcasecmp( d, "offset" ) || !strcasecmp( d, "delay" ) )
	{
		av_int( v );
		if( v > 0 )
			sc->offset = v;
		else
			warn( "Stats offset must be > 0, value %d given.", v );
	}
	else if( !strcasecmp( d, "size" ) || !strcasecmp( d, "hashSize" ) )
	{
		// 0 means default
		if( !( sc->hsize = hash_size( av->val ) ) )
			return -1;
	}
	else
		return -1;

	return 0;
}


#undef bprintf

