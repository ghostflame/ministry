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
	"sec", "tval", "tspec", "msec", "usec", "nsec"
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
	va_list args;
	int l, max;

	// are we ready for a new buffer?
	if( t->bp->len > IO_BUF_HWMK )
	{
		io_buf_send( t->bp );

		if( !( t->bp = mem_new_buf( IO_BUF_SZ ) ) )
		{
			fatal( "Could not allocate a new IOBUF." );
			return;
		}
	}

	// copy the prefix
	memcpy( t->bp->buf + t->bp->len, t->prefix, t->prlen );
	t->bp->len += t->prlen;

	// we need to leave room for the timestamp
	max = t->bp->sz - t->bp->len - t->tsbufsz;

	// write the variable part
	va_start( args, fmt );
	l = vsnprintf( t->bp->buf + t->bp->len, max, fmt, args );
	va_end( args );

	// check for truncation
	if( l > max )
		l = max;

	t->bp->len += l;

	// add the timestamp and newline
	memcpy( t->bp->buf + t->bp->len, t->tsbuf, t->tsbufsz );
	t->bp->len += t->tsbufsz;

	// keep count
	t->paths++;
}


// TIMESTAMP FUNCTIONS
void stats_tsf_sec( ST_THR *t )
{
	t->tsbufsz = snprintf( t->tsbuf, TSBUF_SZ, " %ld\n", t->now.tv_sec );
}

void stats_tsf_msval( ST_THR *t )
{
	t->tsbufsz = snprintf( t->tsbuf, TSBUF_SZ, " %ld.%03ld\n", t->now.tv_sec, t->now.tv_nsec / 1000000 );
}

void stats_tsf_tval( ST_THR *t )
{
	t->tsbufsz = snprintf( t->tsbuf, TSBUF_SZ, " %ld.%06ld\n", t->now.tv_sec, t->now.tv_nsec / 1000 );
}

void stats_tsf_tspec( ST_THR *t )
{
	t->tsbufsz = snprintf( t->tsbuf, TSBUF_SZ, " %ld.%09ld\n", t->now.tv_sec, t->now.tv_nsec );
}

void stats_tsf_msec( ST_THR *t )
{
	t->tsbufsz = snprintf( t->tsbuf, TSBUF_SZ, " %ld\n", t->tval / 1000000 );
}

void stats_tsf_usec( ST_THR *t )
{
	t->tsbufsz = snprintf( t->tsbuf, TSBUF_SZ, " %ld\n", t->tval / 1000 );
}

void stats_tsf_nsec( ST_THR *t )
{
	t->tsbufsz = snprintf( t->tsbuf, TSBUF_SZ, " %ld\n", t->tval );
}




// configure the line buffer of a thread from a prefix config
void stats_set_bufs( ST_THR *t, ST_CFG *c, int64_t tval )
{
	// only set the timestamp buffer if we need to
	if( tval )
	{
		// grab the timestamp
		t->tval = tval;
		llts( tval, t->now );

		// and run the relevant function
		(*(ctl->stats->ts_fp))( t );
	}

	// default to our own config
	if( !c )
		c = t->conf;

	t->prefix = c->prefix;
	t->prlen  = c->prlen;
	t->paths  = 0;
	t->points = 0;

	// grab a new buffer
	if( !t->bp && !( t->bp = mem_new_buf( IO_BUF_SZ ) ) )
	{
		fatal( "Could not allocate a new IOBUF." );
		return;
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
	p = t->paths;

	bprintf( t, "%s.active %d", t->wkrstr, p );
	bprintf( t, "%s.points %d", t->wkrstr, t->points );

	if( t->conf->type == STATS_TYPE_STATS )
		bprintf( t, "%s.workspace %d", t->wkrstr, t->wkspcsz );

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
	bprintf( t, "%s.self_paths %ld", t->wkrstr, t->paths + 1 );
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
}


#define st_thr_time( _name )		clock_gettime( CLOCK_REALTIME, &(t->_name) )


void stats_stats_pass( ST_THR *t )
{
	DHASH *d;
	int i;

#ifdef DEBUG
	debug( "[%02d] Stats claim", t->id );
#endif

	st_thr_time( steal );

	// take the data
	for( i = 0; i < t->conf->hsize; i++ )
		if( ( i % t->max ) == t->id )
			for( d = t->conf->data[i]; d; d = d->next )
				if( d->valid && d->in.points )
				{
					lock_stats( d );

					d->proc.points = d->in.points;
					d->in.points   = NULL;

					unlock_stats( d );
				}
				else if( d->empty >= 0 )
					d->empty++;

#ifdef DEBUG
	debug( "[%02d] Stats report", t->id );
#endif

	st_thr_time( stats );

	// and report it
	for( i = 0; i < t->conf->hsize; i++ )
		if( ( i % t->max ) == t->id )
			for( d = t->conf->data[i]; d; d = d->next )
				if( d->valid && d->proc.points )
				{
					if( d->empty > 0 )
						d->empty = 0;

					stats_report_one( t, d );
				}

	// and work out how long that took
	st_thr_time( done );
}



void stats_adder_pass( ST_THR *t )
{
	DHASH *d;
	int i;

#ifdef DEBUG
	debug( "[%02d] Adder claim", t->id );
#endif

	st_thr_time( steal );

	// take the data
	for( i = 0; i < t->conf->hsize; i++ )
		if( ( i % t->max ) == t->id )
			for( d = t->conf->data[i]; d; d = d->next )
				if( d->valid && d->in.sum.count > 0 )
				{
					lock_adder( d );

					d->proc.sum     = d->in.sum;
					d->in.sum.total = 0;
					d->in.sum.count = 0;

					unlock_adder( d );
				}
				else if( d->empty >= 0 )
					d->empty++;

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
			for( d = t->conf->data[i]; d; d = d->next )
				if( d->valid && d->proc.sum.count > 0 )
				{
					if( d->empty > 0 )
						d->empty = 0;

					bprintf( t, "%s %f", d->path, d->proc.sum.total );

					// keep count
					t->points += d->proc.sum.count;

					// and zero that
					d->proc.sum.count = 0;
				}


	// and work out how long that took
	st_thr_time( done );
}


void stats_gauge_pass( ST_THR *t )
{
	DHASH *d;
	int i;

#ifdef DEBUG
	debug( "[%02d] Gauge claim", t->id );
#endif

	st_thr_time( steal );

	// take the data
	for( i = 0; i < t->conf->hsize; i++ )
		if( ( i % t->max ) == t->id )
			for( d = t->conf->data[i]; d; d = d->next )
				if( d->valid && d->in.sum.count ) 
				{
					lock_gauge( d );

					d->proc.sum     = d->in.sum;
					// don't reset the gauge, just the count
					d->in.sum.count = 0;

					unlock_gauge( d );
				}
				else if( d->empty >= 0 )
					d->empty++;

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
				bprintf( t, "%s %f", d->path, d->proc.sum.total );

				if( d->proc.sum.count )
				{
					if( d->empty > 0 )
						d->empty = 0;

					// keep count
					t->points += d->proc.sum.count;

					// and zero that
					d->proc.sum.count = 0;
				}
			}

	// how long did all that take?
	st_thr_time( done );
}


#undef st_thr_time




void thread_pass( int64_t tval, void *arg )
{
	ST_THR *t = (ST_THR *) arg;

	// set up bufs and such
	stats_set_bufs( t, NULL, tval );

	// do the work
	(*(t->conf->statfn))( t );

	// and report on ourself, except self-stats
	if( t->conf->type != STATS_TYPE_SELF )
		stats_thread_report( t );

	// send any outstanding data
	io_buf_send( t->bp );
	t->bp = NULL;
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

	// free an existing
	if( c->prefix )
		free( c->prefix );

	len = strlen( s );

	if( len > 0 )
	{
		// do we need a dot?
		if( s[len - 1] != '.' )
			dot = 1;
	}

	c->prlen = len + dot;

	// include space for a dot
	c->prefix = (char *) allocz( c->prlen + 1 );
	memcpy( c->prefix, s, len );

	if( dot )
		c->prefix[len] = '.';
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

		// timestamp buffer
		t->tsbuf = perm_str( TSBUF_SZ );

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


tsf_fn *stats_tsf_fns[STATS_TSF_MAX] =
{
	&stats_tsf_sec,
	&stats_tsf_tval,
	&stats_tsf_tspec,
	&stats_tsf_msec,
	&stats_tsf_usec,
	&stats_tsf_nsec
};


void stats_init( void )
{
	struct timespec ts;

	// set our timestamps format
	ctl->stats->ts_fp = stats_tsf_fns[ctl->stats->ts_fmt];

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

	// default to seconds format
	s->ts_fmt = STATS_TSF_SEC;

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
		else if( attIs( "timestamps" ) || attIs( "tsformat" ) )
		{
			for( i = 0; i < STATS_TSF_MAX; i++ )
				if( valIs( stats_tsf_names[i] ) )
				{
					ctl->stats->ts_fmt = i;
					return 0;
				}

			warn( "Timestamp format %s not recognised.", av->val );
			return -1;
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
		if( !( sc->enable = config_bool( av ) ) )
			debug( "Stats type %s disabled.", sc->name );
	}
	else if( !strcasecmp( d, "prefix" ) )
	{
		stats_prefix( sc, av->val );
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

