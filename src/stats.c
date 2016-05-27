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

const char *stats_tsf_names[STATS_TSF_MAX] =
{
	"sec", "msec", "usec", "nsec", "dotmsec", "dotusec", "dotnsec"
};





static inline int cmp_floats( const void *p1, const void *p2 )
{
	float *f1, *f2;

	f1 = (float *) p1;
	f2 = (float *) p2;

	return ( *f1 > *f2 ) ? 1  :
	       ( *f2 < *f1 ) ? -1 : 0;
}



// a little caution so we cannot write off the
// end of the buffer
void bprintf( ST_THR *t, char *fmt, ... )
{
	int i, l, total;
	va_list args;
	TSET *s;

	// write the variable part into the thread's path buffer
	va_start( args, fmt );
	l = vsnprintf( t->path->buf, t->path->sz, fmt, args );
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


// send all remaining buffers
void stats_send_bufs( ST_THR *t )
{
	int i;

	for( i = 0; i < ctl->tgt->set_count; i++ )
		if( t->bp[i] )
		{
			target_buf_send( ctl->tgt->setarr[i], t->bp[i] );
			t->bp[i] = NULL;
		}
}



// TIMESTAMP FUNCTIONS
void stats_tsf_sec( BUF *ts, int64_t tval )
{
	struct timespec now;

	llts( tval, now );
	ts->len = snprintf( ts->buf, TSBUF_SZ, " %ld\n", now.tv_sec );
}

void stats_tsf_msec( BUF *ts, int64_t tval )
{
	ts->len = snprintf( ts->buf, TSBUF_SZ, " %ld\n", tval / 1000000 );
}

void stats_tsf_usec( BUF *ts, int64_t tval )
{
	ts->len = snprintf( ts->buf, TSBUF_SZ, " %ld\n", tval / 1000 );
}

void stats_tsf_nsec( BUF *ts, int64_t tval )
{
	ts->len = snprintf( ts->buf, TSBUF_SZ, " %ld\n", tval );
}

void stats_tsf_dotmsec( BUF *ts, int64_t tval )
{
	struct timespec now;

	llts( tval, now );
	ts->len = snprintf( ts->buf, TSBUF_SZ, " %ld.%03ld\n", now.tv_sec, now.tv_nsec / 1000000 );
}

void stats_tsf_dotusec( BUF *ts, int64_t tval )
{
	struct timespec now;

	llts( tval, now );
	ts->len = snprintf( ts->buf, TSBUF_SZ, " %ld.%06ld\n", now.tv_sec, now.tv_nsec / 1000 );
}

void stats_tsf_dotnsec( BUF *ts, int64_t tval )
{
	struct timespec now;

	llts( tval, now );
	ts->len = snprintf( ts->buf, TSBUF_SZ, " %ld.%09ld\n", now.tv_sec, now.tv_nsec );
}




// configure the line buffer of a thread from a prefix config
void stats_set_bufs( ST_THR *t, ST_CFG *c, int64_t tval )
{
	TSET *s;
	int i;

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
			(*(s->stype->tsfp))( t->ts[i], tval );

		// grab a new buffer
		if( !t->bp[i] && !( t->bp[i] = mem_new_buf( IO_BUF_SZ ) ) )
		{
			fatal( "Could not allocate a new IOBUF." );
			return;
		}
	}
}


void stats_report_one( ST_THR *t, DHASH *d )
{
	PTLIST *list, *p;
	int i, ct, idx;
	ST_THOLD *thr;
	float sum;

	// grab the points list
	list = d->proc.points;
	d->proc.points = NULL;

	// count the points
	for( ct = 0, p = list; p; p = p->next )
		ct += p->count;

	// anything to do?
	if( ct == 0 )
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
			float *ws;

			// double it until we have enough
			while( ct > sz )
				sz *= 2;

			if( !( ws = (float *) allocz( sz * sizeof( float ) ) ) )
			{
				fatal( "Could not allocate new workbuf of %d floats.", sz );
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
			memcpy( t->wkspc + i, p->vals, p->count * sizeof( float ) );
			i += p->count;
		}
	}

	sum = 0;
	kahan_summation( t->wkspc, ct, &sum );

	// median offset
	idx = ct / 2;

	// and sort them
	qsort( t->wkspc, ct, sizeof( float ), cmp_floats );

	bprintf( t, "%s.count %d",  d->path, ct );
	bprintf( t, "%s.mean %f",   d->path, sum / (float) ct );
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

	mem_free_point_list( list );

	// keep count
	t->points += ct;
	t->active++;
}



void stats_stats_pass( ST_THR *t, int64_t tval )
{
	struct timespec tv[4];
	double intvpc;
	int64_t nsec;
	DHASH *d;
	int i;

#ifdef DEBUG
	debug( "[%02d] Stats claim", t->id );
#endif

	clock_gettime( CLOCK_REALTIME, &(tv[0]) );

	// take the data
	for( i = 0; i < t->conf->hsize; i++ )
		if( ( i % t->max ) == t->id )
			for( d = t->conf->data[i]; d; d = d->next )
				if( d->len && d->in.points )
				{
					lock_stats( d );

					d->proc.points = d->in.points;
					d->in.points   = NULL;

					unlock_stats( d );
				}
				else if( d->empty >= 0 )
					d->empty++;

#ifdef CALC_JITTER
	clock_gettime( CLOCK_REALTIME, &(tv[1]) );

	// sleep a bit to avoid contention
	usleep( 1000 + ( random( ) % 30011 ) );
#endif

#ifdef DEBUG
	debug( "[%02d] Stats report", t->id );
#endif

	clock_gettime( CLOCK_REALTIME, &(tv[2]) );

	t->points = 0;
	t->active = 0;

	// and report it
	for( i = 0; i < t->conf->hsize; i++ )
		if( ( i % t->max ) == t->id )
			for( d = t->conf->data[i]; d; d = d->next )
				if( d->len && d->proc.points )
				{
					if( d->empty > 0 )
						d->empty = 0;

					stats_report_one( t, d );
				}

	// and work out how long that took
	clock_gettime( CLOCK_REALTIME, &(tv[3]) );

	// report some self stats?
	if( !ctl->stats->self->enable )
		return;

	stats_set_bufs( t, ctl->stats->self, 0 );

	bprintf( t, "%s.points %d",    t->wkrstr, t->points );
	bprintf( t, "%s.active %d",    t->wkrstr, t->active );
	bprintf( t, "%s.workspace %d", t->wkrstr, t->wkspcsz );

	nsec = tsll( tv[0] ) - tval;
	bprintf( t, "%s.delay %lu",    t->wkrstr, nsec / 1000 );

#ifdef CALC_JITTER
	nsec = tsll( tv[1] ) - tsll( tv[0] );
#else
	nsec = tsll( tv[2] ) - tsll( tv[0] );
#endif
	bprintf( t, "%s.steal %lu",    t->wkrstr, nsec / 1000 );

	nsec = tsll( tv[3] ) - tsll( tv[2] );
	bprintf( t, "%s.stats %lu",    t->wkrstr, nsec / 1000 );

	nsec = tsll( tv[3] ) - tval;
	bprintf( t, "%s.usec %lu",     t->wkrstr, nsec / 1000 );

	// calculate percentage of interval
	intvpc  = (double) ( nsec / 10 );
	intvpc /= (double) t->conf->period;
	bprintf( t, "%s.interval_usage %.3f", t->wkrstr, intvpc );
}



void stats_adder_pass( ST_THR *t, int64_t tval )
{
	struct timespec tv[4];
	double intvpc;
	int64_t nsec;
	DHASH *d;
	int i;

#ifdef DEBUG
	debug( "[%02d] Adder claim", t->id );
#endif

	clock_gettime( CLOCK_REALTIME, &(tv[0]) );

	// take the data
	for( i = 0; i < t->conf->hsize; i++ )
		if( ( i % t->max ) == t->id )
			for( d = t->conf->data[i]; d; d = d->next )
				if( d->in.sum.count > 0 )
				{
					lock_adder( d );

					d->proc.sum     = d->in.sum;
					d->in.sum.total = 0;
					d->in.sum.count = 0;

					unlock_adder( d );
				}
				else if( d->empty >= 0 )
					d->empty++;

	clock_gettime( CLOCK_REALTIME, &(tv[1]) );

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

#ifdef CALC_JITTER
	//debug( "[%02d] Sleeping a little before processing.", t->id );

	// sleep a short time to avoid contention
	usleep( 1000 + ( random( ) % 30011 ) );
#endif

#ifdef DEBUG
	debug( "[%02d] Adder report", t->id );
#endif

	clock_gettime( CLOCK_REALTIME, &(tv[2]) );

	// zero the counters
	t->points = 0;
	t->active = 0;

	// and report it
	for( i = 0; i < t->conf->hsize; i++ )
		if( ( i % t->max ) == t->id )
			for( d = t->conf->data[i]; d; d = d->next )
				if( d->proc.sum.count > 0 )
				{
					if( d->empty > 0 )
						d->empty = 0;

					bprintf( t, "%s %f", d->path, d->proc.sum.total );

					// keep count
					t->points += d->proc.sum.count;
					t->active++;

					// and zero that
					d->proc.sum.count = 0;
				}


	// and work out how long that took
	clock_gettime( CLOCK_REALTIME, &(tv[3]) );

	// report some self stats?
	if( !ctl->stats->self->enable )
		return;

	stats_set_bufs( t, ctl->stats->self, 0 );

	bprintf( t, "%s.points %d", t->wkrstr, t->points );
	bprintf( t, "%s.active %d", t->wkrstr, t->active );

	nsec = tsll( tv[0] ) - tval;
	bprintf( t, "%s.delay %lu", t->wkrstr, nsec / 1000 );

	nsec = tsll( tv[1] ) - tsll( tv[0] );
	bprintf( t, "%s.steal %lu", t->wkrstr, nsec / 1000 );

	nsec = tsll( tv[3] ) - tsll( tv[2] );
	bprintf( t, "%s.stats %lu", t->wkrstr, nsec / 1000 );

	nsec = tsll( tv[3] ) - tval;
	bprintf( t, "%s.usec %lu",  t->wkrstr, nsec / 1000 );

	// calculate percentage of interval
	intvpc  = (double) ( nsec / 10 );
	intvpc /= (double) t->conf->period;
	bprintf( t, "%s.interval_usage %.3f", t->wkrstr, intvpc );
}


void stats_gauge_pass( ST_THR *t, int64_t tval )
{
	struct timespec tv[4];
	double intvpc;
	int64_t nsec;
	DHASH *d;
	int i;

#ifdef DEBUG
	debug( "[%02d] Gauge claim", t->id );
#endif

	clock_gettime( CLOCK_REALTIME, &(tv[0]) );

	// take the data
	for( i = 0; i < t->conf->hsize; i++ )
		if( ( i % t->max ) == t->id )
			for( d = t->conf->data[i]; d; d = d->next )
				if( d->in.sum.count ) 
				{
					lock_gauge( d );

					d->proc.sum     = d->in.sum;
					// don't reset the gauge, just the count
					d->in.sum.count = 0;

					unlock_gauge( d );
				}
				else if( d->empty >= 0 )
					d->empty++;


#ifdef CALC_JITTER
	clock_gettime( CLOCK_REALTIME, &(tv[1]) );

	// sleep a bit to avoid contention
	usleep( 1000 + ( random( ) % 30011 ) );
#endif

#ifdef DEBUG
	debug( "[%02d] Gauge report", t->id );
#endif

	clock_gettime( CLOCK_REALTIME, &(tv[2]) );

	t->active = 0;
	t->points = 0;

	// and report it
	for( i = 0; i < t->conf->hsize; i++ )
		if( ( i % t->max ) == t->id )
			for( d = t->conf->data[i]; d; d = d->next )
			{
				// we report gauges anyway, updated or not
				bprintf( t, "%s %f", d->path, d->proc.sum.total );

				if( d->proc.sum.count )
				{
					if( d->empty > 0 )
						d->empty = 0;

					// keep count
					t->points += d->proc.sum.count;
					t->active++;

					// and zero that
					d->proc.sum.count = 0;
				}
			}

	clock_gettime( CLOCK_REALTIME, &(tv[3]) );

	// report some self stats?
	if( !ctl->stats->self->enable )
		return;

	stats_set_bufs( t, ctl->stats->self, 0 );

	bprintf( t, "%s.points %d", t->wkrstr, t->points );
	bprintf( t, "%s.active %d", t->wkrstr, t->active );

	nsec = tsll( tv[0] ) - tval;
	bprintf( t, "%s.delay %lu", t->wkrstr, nsec / 1000 );

#ifdef CALC_JITTER
	nsec = tsll( tv[1] ) - tsll( tv[0] );
#else
	nsec = tsll( tv[2] ) - tsll( tv[0] );
#endif
	bprintf( t, "%s.steal %lu", t->wkrstr, nsec / 1000 );

	nsec = tsll( tv[3] ) - tsll( tv[2] );
	bprintf( t, "%s.stats %lu", t->wkrstr, nsec / 1000 );

	nsec = tsll( tv[3] ) - tval;
	bprintf( t, "%s.usec %lu",  t->wkrstr, nsec / 1000 );

	// calculate percentage of interval
	intvpc  = (double) ( nsec / 10 );
	intvpc /= (double) t->conf->period;
	bprintf( t, "%s.interval_usage %.3f", t->wkrstr, intvpc );
}



void thread_pass( int64_t tval, void *arg )
{
	ST_THR *t = (ST_THR *) arg;
	int i;

	// set up bufs and such
	stats_set_bufs( t, NULL, tval );

	// do the work
	(*(t->conf->statfn))( t, tval );

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
		thread_throw( &stats_loop, &(cf->ctls[i]) );

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
	if( c->hsize < 0 )
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
		l = snprintf( wkrstrbuf, 128, "workers.%s.%d", c->name, t->id );
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
			t->wkbuf   = (float *) allocz( t->wkspcsz * sizeof( float ) );
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
	s->stats->hsize   = -1;
	s->stats->enable  = 1;
	stats_prefix( s->stats, DEFAULT_STATS_PREFIX );

	s->adder          = (ST_CFG *) allocz( sizeof( ST_CFG ) );
	s->adder->threads = DEFAULT_ADDER_THREADS;
	s->adder->statfn  = &stats_adder_pass;
	s->adder->period  = DEFAULT_STATS_MSEC;
	s->adder->type    = STATS_TYPE_ADDER;
	s->adder->dtype   = DATA_TYPE_ADDER;
	s->adder->name    = stats_type_names[STATS_TYPE_ADDER];
	s->adder->hsize   = -1;
	s->adder->enable  = 1;
	stats_prefix( s->stats, DEFAULT_ADDER_PREFIX );

	s->gauge          = (ST_CFG *) allocz( sizeof( ST_CFG ) );
	s->gauge->threads = DEFAULT_GAUGE_THREADS;
	s->gauge->statfn  = &stats_gauge_pass;
	s->gauge->period  = DEFAULT_STATS_MSEC;
	s->gauge->type    = STATS_TYPE_GAUGE;
	s->gauge->dtype   = DATA_TYPE_GAUGE;
	s->gauge->name    = stats_type_names[STATS_TYPE_GAUGE];
	s->gauge->hsize   = -1;
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

	return s;
}

int stats_config_line( AVP *av )
{
	char *d, *pm, *lbl, *fmt, thrbuf[64];
	int i, t, l, mid, top;
	ST_THOLD *th;
	STAT_CTL *s;
	ST_CFG *sc;
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
	else
		return -1;

	if( !strcasecmp( d, "threads" ) )
	{
		t = atoi( av->val );
		if( t > 0 )
			sc->threads = t;
		else
			warn( "Stats threads must be > 0, value %d given.", t );
	}
	else if( !strcasecmp( d, "enable" ) )
	{
		if( valIs( "y" ) || valIs( "yes" ) || atoi( av->val ) )
			sc->enable = 1;
		else
		{
			debug( "Stats type %s disabled.", sc->name );
			sc->enable = 0;
		}
	}
	else if( !strcasecmp( d, "prefix" ) )
	{
		stats_prefix( sc, av->val );
		debug( "%s prefix set to '%s'", sc->name, sc->prefix->buf );
	}
	else if( !strcasecmp( d, "period" ) )
	{
		t = atoi( av->val );
		if( t > 0 )
			sc->period = t;
		else
			warn( "Stats period must be > 0, value %d given.", t );
	}
	else if( !strcasecmp( d, "offset" ) || !strcasecmp( d, "delay" ) )
	{
		t = atoi( av->val );
		if( t > 0 )
			sc->offset = t;
		else
			warn( "Stats offset must be > 0, value %d given.", t );
	}
	else if( !strcasecmp( d, "size" ) || !strcasecmp( d, "hashsize" ) )
	{
		if( valIs( "tiny" ) )
			sc->hsize = MEM_HSZ_TINY;
		else if( valIs( "small" ) )
			sc->hsize = MEM_HSZ_SMALL;
		else if( valIs( "medium" ) )
			sc->hsize = MEM_HSZ_MEDIUM;
		else if( valIs( "large" ) )
			sc->hsize = MEM_HSZ_LARGE;
		else if( valIs( "xlarge" ) )
			sc->hsize = MEM_HSZ_XLARGE;
		else if( valIs( "x2large" ) )
			sc->hsize = MEM_HSZ_X2LARGE;
		else
		{
			if( !isdigit( av->val[0] ) )
			{
				warn( "Unrecognised hash table size '%s'", av->val );
				return -1;
			}
			t = atoi( av->val );
			if( t == 0 )
			{
				warn( "Cannot set zero size hash table." );
				return -1;
			}
			// < 0 means default
			sc->hsize = t;
		}
	}
	else
		return -1;

	return 0;
}


#undef bprintf

