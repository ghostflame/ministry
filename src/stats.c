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



inline int cmp_floats( const void *p1, const void *p2 )
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
	int i, ct, tind, thr, med;
	PTLIST *list, *p;
	char *prfx, *ul;
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
	med = ct / 2;

	// and sort them
	qsort( t->wkspc, ct, sizeof( float ), cmp_floats );

	bprintf( "%s.count %d",  d->path, ct );
	bprintf( "%s.mean %f",   d->path, sum / (float) ct );
	bprintf( "%s.upper %f",  d->path, t->wkspc[ct-1] );
	bprintf( "%s.lower %f",  d->path, t->wkspc[0] );
	bprintf( "%s.median %f", d->path, t->wkspc[med] );

	// variable thresholds
	for( i = 0; i < ctl->stats->thr_count; i++ )
	{
		// exclude lunacy
		if( !( thr = ctl->stats->thresholds[i] ) )
			continue;

		// decide on a string
		ul = ( thr < 50 ) ? "lower" : "upper";

		// find the right index into our values
		tind = ( ct * thr ) / 100;

   		bprintf( "%s.%s_%d %f", d->path, ul, thr, t->wkspc[tind] );
	}

	mem_free_point_list( list );
	*buf = b;

	return ct;
}




void stats_stats_pass( uint64_t tval, void *arg )
{
	struct timeval tva, tvb, tvc;
	uint64_t usec, steal, stats;
	char *prfx;
	time_t ts;
	ST_THR *c;
	DHASH *d;
	IOBUF *b;
	int i;

	c  = (ST_THR *) arg;
	ts = (time_t) ( tval / 1000000 );

#ifdef DEBUG
	debug( "[%02d] Stats claim", c->id );
#endif

	// take the data
	for( i = 0; i < ctl->mem->hashsize; i++ )
		if( ( i % c->max ) == c->id )
			for( d = c->conf->data[i]; d; d = d->next )
			{
				lock_stats( d );

				d->proc.points = d->in.points;
				d->in.points   = NULL;

				unlock_stats( d );
			}

	gettimeofday( &tva, NULL );

	// sleep a bit to avoid contention
	usleep( 1000 + ( random( ) % 30011 ) );

#ifdef DEBUG
	debug( "[%02d] Stats report", c->id );
#endif

	gettimeofday( &tvb, NULL );

	c->points = 0;
	c->active = 0;

	b = mem_new_buf( IO_BUF_SZ );

	// and report it
	for( i = 0; i < ctl->mem->hashsize; i++ )
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
				else if( d->empty >= 0 )
					d->empty++;

	// and work out how long that took
	gettimeofday( &tvc, NULL );
	steal = tvll( tva ) - tval;
	stats = tvll( tvc ) - tvll( tvb );
	usec  = tvll( tvc ) - tval;

	// report some self stats
	prfx = ctl->stats->self->prefix;

	bprintf( "workers.%s.%d.points %d",    c->conf->name, c->id, c->points );
	bprintf( "workers.%s.%d.active %d",    c->conf->name, c->id, c->active );
	bprintf( "workers.%s.%d.workspace %d", c->conf->name, c->id, c->wkspcsz );
	bprintf( "workers.%s.%d.steal %lu",    c->conf->name, c->id, steal );
	bprintf( "workers.%s.%d.stats %lu",    c->conf->name, c->id, stats );
	bprintf( "workers.%s.%d.usec %lu",     c->conf->name, c->id, usec );

	io_buf_send( b );
}



void stats_adder_pass( uint64_t tval, void *arg )
{
	struct timeval tva, tvb, tvc;
	uint64_t usec, steal, stats;
	char *prfx;
	ST_THR *c;
	time_t ts;
	DHASH *d;
	IOBUF *b;
	int i;

	c    = (ST_THR *) arg;
	ts   = (time_t) ( tval / 1000000 );
	prfx = ctl->stats->adder->prefix;

#ifdef DEBUG
	debug( "[%02d] Adder claim", c->id );
#endif

	// take the data
	for( i = 0; i < ctl->mem->hashsize; i++ )
		if( ( i % c->max ) == c->id )
			for( d = c->conf->data[i]; d; d = d->next )
			{
				lock_adder( d );

				d->proc.sum     = d->in.sum;
				d->in.sum.total = 0;
				d->in.sum.count = 0;

				unlock_adder( d );
			}

	gettimeofday( &tva, NULL );

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

	// sleep a short time to avoid contention
	usleep( 1000 + ( random( ) % 30011 ) );

#ifdef DEBUG
	debug( "[%02d] Adder report", c->id );
#endif

	gettimeofday( &tvb, NULL );

	// zero the counters
	c->points = 0;
	c->active = 0;

	b = mem_new_buf( IO_BUF_SZ );

	// and report it
	for( i = 0; i < ctl->mem->hashsize; i++ )
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
				}
				else if( d->empty >= 0 )
					d->empty++;


	// and work out how long that took
	gettimeofday( &tvc, NULL );
	steal = tvll( tva ) - tval;
	stats = tvll( tvc ) - tvll( tvb );
	usec  = tvll( tvc ) - tval;

	// report some self stats
	prfx = ctl->stats->self->prefix;

	bprintf( "workers.%s.%d.points %d", c->conf->name, c->id, c->points );
	bprintf( "workers.%s.%d.active %d", c->conf->name, c->id, c->active );
	bprintf( "workers.%s.%d.steal %lu", c->conf->name, c->id, steal );
	bprintf( "workers.%s.%d.stats %lu", c->conf->name, c->id, stats );
	bprintf( "workers.%s.%d.usec %lu",  c->conf->name, c->id, usec );

	io_buf_send( b );
}


void stats_gauge_pass( uint64_t tval, void *arg )
{
	struct timeval tva, tvb, tvc;
	uint64_t usec, steal, stats;
	char *prfx;
	ST_THR *c;
	time_t ts;
	DHASH *d;
	IOBUF *b;
	int i;

	c    = (ST_THR *) arg;
	ts   = (time_t) ( tval / 1000000 );
	prfx = ctl->stats->gauge->prefix;

#ifdef DEBUG
	debug( "[%02d] Gauge claim", c->id );
#endif

	// take the data
	for( i = 0; i < ctl->mem->hashsize; i++ )
		if( ( i % c->max ) == c->id )
			for( d = c->conf->data[i]; d; d = d->next )
			{
				lock_adder( d );

				d->proc.sum     = d->in.sum;
				// don't reset the gauge, just the count
				d->in.sum.count = 0;

				unlock_adder( d );
			}


	gettimeofday( &tva, NULL );

	// sleep a bit to avoid contention
	usleep( 1000 + ( random( ) % 30011 ) );

#ifdef DEBUG
	debug( "[%02d] Gauge report", c->id );
#endif

	gettimeofday( &tvb, NULL );

	c->active = 0;
	c->points = 0;

	b = mem_new_buf( IO_BUF_SZ );

	// and report it
	for( i = 0; i < ctl->mem->hashsize; i++ )
		if( ( i % c->max ) == c->id )
			for( d = c->conf->data[i]; d; d = d->next )
				if( d->proc.sum.count )
				{
					if( d->empty > 0 )
						d->empty = 0;

					bprintf( "%s %f", d->path, d->proc.sum.total );

					// keep count
					c->points += d->proc.sum.count;
					c->active++;
				}
				else if( d->empty >= 0 )
					d->empty++;


	// and work out how long that took
	gettimeofday( &tvc, NULL );
	steal = tvll( tva ) - tval;

	// and work out how long that took
	gettimeofday( &tvc, NULL );
	steal = tvll( tva ) - tval;
	stats = tvll( tvc ) - tvll( tvb );
	usec  = tvll( tvc ) - tval;

	// report some self stats
	prfx = ctl->stats->self->prefix;

	bprintf( "workers.%s.%d.points %d", c->conf->name, c->id, c->points );
	bprintf( "workers.%s.%d.active %d", c->conf->name, c->id, c->active );
	bprintf( "workers.%s.%d.steal %lu", c->conf->name, c->id, steal );
	bprintf( "workers.%s.%d.stats %lu", c->conf->name, c->id, stats );
	bprintf( "workers.%s.%d.usec %lu",  c->conf->name, c->id, usec );

	io_buf_send( b );
}






#define stats_report_mtype( nm, mt )		bytes = ((uint64_t) mt->alloc_sz) * ((uint64_t) mt->total); \
											bprintf( "mem.%s.free %u",  nm, mt->fcount ); \
											bprintf( "mem.%s.alloc %u", nm, mt->total ); \
											bprintf( "mem.%s.kb %lu",   nm, bytes >> 10 )

// report our own pass
void stats_self_pass( uint64_t tval, void *arg )
{
	struct timeval now;
	uint64_t bytes;
	double upt;
	char *prfx;
	time_t ts;
	IOBUF *b;

	ts = (time_t) ( tval / 1000000 );

	now.tv_sec  = ts;
	now.tv_usec = tval % 1000000;

	prfx = ctl->stats->self->prefix;
	tv_diff( now, ctl->init_time, &upt );

	b = mem_new_buf( IO_BUF_SZ );

	// TODO - more stats
	bprintf( "uptime %.3f", upt );
	bprintf( "paths.stats.curr %d", ctl->stats->stats->dcurr );
	bprintf( "paths.stats.gc %d",   ctl->stats->stats->gc_count );
	bprintf( "paths.adder.curr %d", ctl->stats->adder->dcurr );
	bprintf( "paths.adder.gc %d",   ctl->stats->adder->gc_count );
	bprintf( "mem.total.kb %d",     ctl->mem->curr_kb );

	// memory
	stats_report_mtype( "hosts",  ctl->mem->hosts );
	stats_report_mtype( "points", ctl->mem->points );
	stats_report_mtype( "dhash",  ctl->mem->dhash );
	stats_report_mtype( "bufs",   ctl->mem->iobufs );
	stats_report_mtype( "iolist", ctl->mem->iolist );

	io_buf_send( b );
}


#undef stats_report_mtype



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

	if( !cf->enable )
	{
		notice( "Data submission for %s is disabled.", cf->name );
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
	ST_THR *t;
	int i;

	// create the hash structure
	if( alloc_data )
		c->data = (DHASH **) allocz( ctl->mem->hashsize * sizeof( DHASH * ) );

	// convert msec to usec
	c->period *= 1000;
	c->offset *= 1000;
	// offset can't be bigger than period
	c->offset  = c->offset % c->period;

	// grab our name
	c->name = stats_type_names[c->type];

	// make the control structures
	c->ctls = (ST_THR *) allocz( c->threads * sizeof( ST_THR ) );

	for( i = 0; i < c->threads; i++ )
	{
		t = &(c->ctls[i]);

		t->conf   = c;
		t->id     = i;
		t->max    = c->threads;

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
	struct timeval tv;

	stats_init_control( ctl->stats->stats, 1 );
	stats_init_control( ctl->stats->adder, 1 );
	stats_init_control( ctl->stats->gauge, 1 );

	// let's not always seed from 1, eh?
	gettimeofday( &tv, NULL );
	srandom( tv.tv_usec );

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
	s->stats->enable  = 1;

	s->adder          = (ST_CFG *) allocz( sizeof( ST_CFG ) );
	s->adder->threads = DEFAULT_ADDER_THREADS;
	s->adder->loopfn  = &stats_adder_pass;
	s->adder->period  = DEFAULT_STATS_MSEC;
	s->adder->prefix  = stats_prefix( DEFAULT_ADDER_PREFIX );
	s->adder->type    = STATS_TYPE_ADDER;
	s->adder->enable  = 1;

	s->gauge          = (ST_CFG *) allocz( sizeof( ST_CFG ) );
	s->gauge->threads = DEFAULT_GAUGE_THREADS;
	s->gauge->loopfn  = &stats_gauge_pass;
	s->gauge->period  = DEFAULT_STATS_MSEC;
	s->gauge->prefix  = stats_prefix( DEFAULT_GAUGE_PREFIX );
	s->gauge->type    = STATS_TYPE_GAUGE;
	s->gauge->enable  = 1;

	s->self           = (ST_CFG *) allocz( sizeof( ST_CFG ) );
	s->self->threads  = 1;
	s->self->loopfn   = &stats_self_pass;
	s->self->period   = DEFAULT_STATS_MSEC;
	s->self->prefix   = stats_prefix( DEFAULT_SELF_PREFIX );
	s->self->type     = STATS_TYPE_SELF;
	s->self->enable   = 1;

	return s;
}

int stats_config_line( AVP *av )
{
	STAT_CTL *s;
	ST_CFG *sc;
	WORDS wd;
	int i, t;
	char *d;

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
			s->thr_count = wd.wc;
			if( s->thr_count > 20 )
			{
				warn( "Threshold list being truncated at 20" );
				s->thr_count = 20;
			}
			s->thresholds = (int *) allocz( s->thr_count * sizeof( int ) );
			for( i = 0; i < s->thr_count; i++ )
			{
			 	t = atoi( wd.wd[i] );
				// remove lunacy
				if( t < 0 || t == 50 || t >= 100 )
					t = 0;
				s->thresholds[i] = t;
			}

			debug( "Acquired %d thresholds.", s->thr_count );
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
			sc->enable = 0;
	}
	else if( !strcasecmp( d, "prefix" ) )
	{
		free( sc->prefix );
		sc->prefix = stats_prefix( av->val );
		debug( "%s set to '%s'", av->att, sc->prefix );
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
	else
		return -1;

	return 0;
}


#undef bprintf

