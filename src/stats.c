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
	float *f1, *f2;

	f1 = (float *) p1;
	f2 = (float *) p2;

	return ( *f1 > *f2 ) ? 1  :
	       ( *f2 < *f1 ) ? -1 : 0;
}

// this macro is some serious va args abuse
#define bprintf( fmt, ... )			if( b->len > IO_BUF_HWMK ) \
									{ io_buf_send( b ); b = mem_new_buf( IO_BUF_SZ ); } \
									b->len += snprintf( b->buf + b->len, b->sz - b->len, "%s" fmt " %ld\n", prfx, ## __VA_ARGS__, ts )



int stats_report_one( DHASH *d, ST_THR *t, time_t ts, IOBUF **buf )
{
	PTLIST *list, *p;
	int i, ct, idx;
	ST_THOLD *thr;
	char *prfx;
	float sum;
	IOBUF *b;

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
		return 0;
	}

    b    = *buf;
	prfx = ctl->stats->stats->prefix;


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
			// double it until we have enough
			while( ct > t->wkspcsz )
				t->wkspcsz *= 2;

			// free the existing and grab a new chunk
			free( t->wkbuf );
			t->wkbuf = (float *) allocz( t->wkspcsz * sizeof( float ) );

			// make sure
			if( !t->wkbuf )
			{
				fatal( "Could not allocate new workbuf of %d floats.", t->wkspcsz );
				return 0;
			}
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

	bprintf( "%s.count %d",  d->path, ct );
	bprintf( "%s.mean %f",   d->path, sum / (float) ct );
	bprintf( "%s.upper %f",  d->path, t->wkspc[ct-1] );
	bprintf( "%s.lower %f",  d->path, t->wkspc[0] );
	bprintf( "%s.median %f", d->path, t->wkspc[idx] );

	// variable thresholds
	for( thr = ctl->stats->thresholds; thr; thr = thr->next )
	{
		// find the right index into our values
		idx = ( thr->val * ct ) / thr->max;
		bprintf( "%s.%s %f", d->path, thr->label, t->wkspc[idx] );
	}

	mem_free_point_list( list );
	*buf = b;

	return ct;
}



void stats_stats_pass( int64_t tval, void *arg )
{
	struct timespec tv[4];
	int64_t nsec;
	char *prfx;
	time_t ts;
	ST_THR *c;
	DHASH *d;
	IOBUF *b;
	int i;

	c  = (ST_THR *) arg;
	ts = (time_t) tvalts( tval );

#ifdef DEBUG
	debug( "[%02d] Stats claim", c->id );
#endif

	clock_gettime( CLOCK_REALTIME, &(tv[0]) );

	// take the data
	for( i = 0; i < c->conf->hsize; i++ )
		if( ( i % c->max ) == c->id )
			for( d = c->conf->data[i]; d; d = d->next )
				if( d->in.points )
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
	debug( "[%02d] Stats report", c->id );
#endif

	clock_gettime( CLOCK_REALTIME, &(tv[2]) );

	c->points = 0;
	c->active = 0;

	b = mem_new_buf( IO_BUF_SZ );

	// and report it
	for( i = 0; i < c->conf->hsize; i++ )
		if( ( i % c->max ) == c->id )
			for( d = c->conf->data[i]; d; d = d->next )
				if( d->proc.points )
				{
					if( d->empty > 0 )
						d->empty = 0;

					// the buffer can get changed during
					// this function
					c->points += stats_report_one( d, c, ts, &b );
					c->active++;
				}

	// and work out how long that took
	clock_gettime( CLOCK_REALTIME, &(tv[3]) );

	if( ctl->stats->self->enable )
	{
		// report some self stats
		prfx = ctl->stats->self->prefix;

		bprintf( "%s.points %d",    c->wkrstr, c->points );
		bprintf( "%s.active %d",    c->wkrstr, c->active );
		bprintf( "%s.workspace %d", c->wkrstr, c->wkspcsz );

		nsec = tsll( tv[0] ) - tval;
		bprintf( "%s.delay %lu",    c->wkrstr, nsec / 1000 );

#ifdef CALC_JITTER
		nsec = tsll( tv[1] ) - tsll( tv[0] );
#else
		nsec = tsll( tv[2] ) - tsll( tv[0] );
#endif
		bprintf( "%s.steal %lu",    c->wkrstr, nsec / 1000 );

		nsec = tsll( tv[3] ) - tsll( tv[2] );
		bprintf( "%s.stats %lu",    c->wkrstr, nsec / 1000 );

		nsec = tsll( tv[3] ) - tval;
		bprintf( "%s.usec %lu",     c->wkrstr, nsec / 1000 );
	}

	io_buf_send( b );
}



void stats_adder_pass( int64_t tval, void *arg )
{
	struct timespec tv[4];
	int64_t nsec;
	char *prfx;
	ST_THR *c;
	time_t ts;
	DHASH *d;
	IOBUF *b;
	int i;

	c    = (ST_THR *) arg;
	ts   = (time_t) tvalts( tval );
	prfx = ctl->stats->adder->prefix;

#ifdef DEBUG
	debug( "[%02d] Adder claim", c->id );
#endif

	clock_gettime( CLOCK_REALTIME, &(tv[0]) );

	// take the data
	for( i = 0; i < c->conf->hsize; i++ )
		if( ( i % c->max ) == c->id )
			for( d = c->conf->data[i]; d; d = d->next )
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

	//debug( "[%02d] Unlocking adder lock.", c->id );

	// synth thread is waiting for this
	unlock_stthr( c );

	//debug( "[%02d] Trying to lock synth.", c->id );

	// try to lock the synth thread
	lock_synth( );

	//debug( "[%02d] Unlocking synth.", c->id );

	// and then unlock it
	unlock_synth( );

	//debug( "[%02d] Trying to get our own lock back.", c->id );

	// and lock our own again
	lock_stthr( c );

	//debug( "[%02d] Sleeping a little before processing.", c->id );

#ifdef CALC_JITTER
	// sleep a short time to avoid contention
	usleep( 1000 + ( random( ) % 30011 ) );
#endif

#ifdef DEBUG
	debug( "[%02d] Adder report", c->id );
#endif

	clock_gettime( CLOCK_REALTIME, &(tv[2]) );

	// zero the counters
	c->points = 0;
	c->active = 0;

	b = mem_new_buf( IO_BUF_SZ );

	// and report it
	for( i = 0; i < c->conf->hsize; i++ )
		if( ( i % c->max ) == c->id )
			for( d = c->conf->data[i]; d; d = d->next )
				if( d->proc.sum.count > 0 )
				{
					if( d->empty > 0 )
						d->empty = 0;

					bprintf( "%s %f", d->path, d->proc.sum.total );

					// keep count
					c->points += d->proc.sum.count;
					c->active++;

					// and zero that
					d->proc.sum.count = 0;
				}


	// and work out how long that took
	clock_gettime( CLOCK_REALTIME, &(tv[3]) );

	if( ctl->stats->self->enable )
	{
		// report some self stats
		prfx = ctl->stats->self->prefix;

		bprintf( "%s.points %d", c->wkrstr, c->points );
		bprintf( "%s.active %d", c->wkrstr, c->active );

		nsec = tsll( tv[0] ) - tval;
		bprintf( "%s.delay %lu", c->wkrstr, nsec / 1000 );

		nsec = tsll( tv[1] ) - tsll( tv[0] );
		bprintf( "%s.steal %lu", c->wkrstr, nsec / 1000 );

		nsec = tsll( tv[3] ) - tsll( tv[2] );
		bprintf( "%s.stats %lu", c->wkrstr, nsec / 1000 );

		nsec = tsll( tv[3] ) - tval;
		bprintf( "%s.usec %lu",  c->wkrstr, nsec / 1000 );
	}

	io_buf_send( b );
}


void stats_gauge_pass( int64_t tval, void *arg )
{
	struct timespec tv[4];
	int64_t nsec;
	char *prfx;
	ST_THR *c;
	time_t ts;
	DHASH *d;
	IOBUF *b;
	int i;

	c    = (ST_THR *) arg;
	ts   = (time_t) tvalts( tval );
	prfx = ctl->stats->gauge->prefix;

#ifdef DEBUG
	debug( "[%02d] Gauge claim", c->id );
#endif

	clock_gettime( CLOCK_REALTIME, &(tv[0]) );

	// take the data
	for( i = 0; i < c->conf->hsize; i++ )
		if( ( i % c->max ) == c->id )
			for( d = c->conf->data[i]; d; d = d->next )
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
	debug( "[%02d] Gauge report", c->id );
#endif

	clock_gettime( CLOCK_REALTIME, &(tv[2]) );

	c->active = 0;
	c->points = 0;

	b = mem_new_buf( IO_BUF_SZ );

	// and report it
	for( i = 0; i < c->conf->hsize; i++ )
		if( ( i % c->max ) == c->id )
			for( d = c->conf->data[i]; d; d = d->next )
			{
				// we report gauges anyway, updated or not
				bprintf( "%s %f", d->path, d->proc.sum.total );

				if( d->proc.sum.count )
				{
					if( d->empty > 0 )
						d->empty = 0;

					// keep count
					c->points += d->proc.sum.count;
					c->active++;

					// and zero that
					d->proc.sum.count = 0;
				}
			}

	clock_gettime( CLOCK_REALTIME, &(tv[3]) );

	if( ctl->stats->self->enable )
	{
		// report some self stats
		prfx = ctl->stats->self->prefix;

		bprintf( "%s.points %d", c->wkrstr, c->points );
		bprintf( "%s.active %d", c->wkrstr, c->active );

		nsec = tsll( tv[0] ) - tval;
		bprintf( "%s.delay %lu", c->wkrstr, nsec / 1000 );

#ifdef CALC_JITTER
		nsec = tsll( tv[1] ) - tsll( tv[0] );
#else
		nsec = tsll( tv[2] ) - tsll( tv[0] );
#endif
		bprintf( "%s.steal %lu", c->wkrstr, nsec / 1000 );

		nsec = tsll( tv[3] ) - tsll( tv[2] );
		bprintf( "%s.stats %lu", c->wkrstr, nsec / 1000 );

		nsec = tsll( tv[3] ) - tval;
		bprintf( "%s.usec %lu",  c->wkrstr, nsec / 1000 );
	}

	io_buf_send( b );
}




IOBUF *stats_report_mtype( char *name, MTYPE *mt, time_t ts, IOBUF *b )
{
	char *prfx = ctl->stats->self->prefix;
	uint32_t freec, alloc;
	uint64_t bytes;

	// grab some stats under lock
	// we CANNOT bprintf under lock because
	// one of the reported types is buffers
	lock_mem( mt );

	bytes = (uint64_t) mt->alloc_sz * (uint64_t) mt->total;
	alloc = mt->total;
	freec = mt->fcount;

	unlock_mem( mt );

	// and report them
	bprintf( "mem.%s.free %u",  name, freec );
	bprintf( "mem.%s.alloc %u", name, alloc );
	bprintf( "mem.%s.kb %lu",   name, bytes / 1024 );

	return b;
}


IOBUF *stats_report_types( ST_CFG *c, time_t ts, IOBUF *b )
{
	char *prfx = ctl->stats->self->prefix;
	float hr;

	hr = (float) c->dcurr / (float) c->hsize;

	bprintf( "paths.%s.curr %d",         c->name, c->dcurr );
	bprintf( "paths.%s.gc %d",           c->name, c->gc_count );
	bprintf( "paths.%s.hash_ratio %.6f", c->name, hr );

	return b;
}


// report our own pass
void stats_self_pass( int64_t tval, void *arg )
{
	char *prfx = ctl->stats->self->prefix;
	struct timespec now;
	time_t ts;
	IOBUF *b;

	llts( tval, now );
	ts = now.tv_sec;

	b = mem_new_buf( IO_BUF_SZ );

	// TODO - more stats

	// stats types
	b = stats_report_types( ctl->stats->stats, ts, b );
	b = stats_report_types( ctl->stats->adder, ts, b );
	b = stats_report_types( ctl->stats->gauge, ts, b );

	// memory
	b = stats_report_mtype( "hosts",  ctl->mem->hosts,  ts, b );
	b = stats_report_mtype( "points", ctl->mem->points, ts, b );
	b = stats_report_mtype( "dhash",  ctl->mem->dhash,  ts, b );
	b = stats_report_mtype( "bufs",   ctl->mem->iobufs, ts, b );
	b = stats_report_mtype( "iolist", ctl->mem->iolist, ts, b );

	bprintf( "mem.total.kb %d", ctl->mem->curr_kb );
	bprintf( "uptime %.3f", ts_diff( now, ctl->init_time, NULL ) );

	io_buf_send( b );
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
	loop_control( cf->name, cf->loopfn, c, cf->period, 1, cf->offset );

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




// return a copy of a prefix with a trailing .
char *stats_prefix( char *s )
{
	int len, dot = 0;
	char *p;
	
	if( !( len = strlen( s ) ) )
		return strdup( "" );

	// do we need a dot?
	if( s[len - 1] != '.' )
		dot = 1;

	// include space for a dot
	p = (char *) allocz( len + dot + 1 );
	memcpy( p, s, len );

	if( dot )
		p[len] = '.';

	return p;
}




void stats_init_control( ST_CFG *c, int alloc_data )
{
	char wkrstrbuf[128];
	ST_THR *t;
	int i, l;

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
	s->stats->loopfn  = &stats_stats_pass;
	s->stats->period  = DEFAULT_STATS_MSEC;
	s->stats->prefix  = stats_prefix( DEFAULT_STATS_PREFIX );
	s->stats->type    = STATS_TYPE_STATS;
	s->stats->dtype   = DATA_TYPE_STATS;
	s->stats->name    = stats_type_names[STATS_TYPE_STATS];
	s->stats->hsize   = -1;
	s->stats->enable  = 1;

	s->adder          = (ST_CFG *) allocz( sizeof( ST_CFG ) );
	s->adder->threads = DEFAULT_ADDER_THREADS;
	s->adder->loopfn  = &stats_adder_pass;
	s->adder->period  = DEFAULT_STATS_MSEC;
	s->adder->prefix  = stats_prefix( DEFAULT_ADDER_PREFIX );
	s->adder->type    = STATS_TYPE_ADDER;
	s->adder->dtype   = DATA_TYPE_ADDER;
	s->adder->name    = stats_type_names[STATS_TYPE_ADDER];
	s->adder->hsize   = -1;
	s->adder->enable  = 1;

	s->gauge          = (ST_CFG *) allocz( sizeof( ST_CFG ) );
	s->gauge->threads = DEFAULT_GAUGE_THREADS;
	s->gauge->loopfn  = &stats_gauge_pass;
	s->gauge->period  = DEFAULT_STATS_MSEC;
	s->gauge->prefix  = stats_prefix( DEFAULT_GAUGE_PREFIX );
	s->gauge->type    = STATS_TYPE_GAUGE;
	s->gauge->dtype   = DATA_TYPE_GAUGE;
	s->gauge->name    = stats_type_names[STATS_TYPE_GAUGE];
	s->gauge->hsize   = -1;
	s->gauge->enable  = 1;

	s->self           = (ST_CFG *) allocz( sizeof( ST_CFG ) );
	s->self->threads  = 1;
	s->self->loopfn   = &stats_self_pass;
	s->self->period   = DEFAULT_STATS_MSEC;
	s->self->prefix   = stats_prefix( DEFAULT_SELF_PREFIX );
	s->self->type     = STATS_TYPE_SELF;
	s->self->dtype    = -1;
	s->self->name     = stats_type_names[STATS_TYPE_SELF];
	s->self->enable   = 1;

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
		free( sc->prefix );
		sc->prefix = stats_prefix( av->val );
		debug( "%s prefix set to '%s'", sc->name, sc->prefix );
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

