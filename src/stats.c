/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* stats.c - statistics functions and config                               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"



inline int cmp_floats( const void *p1, const void *p2 )
{
	float *f1, *f2;

	f1 = (float *) p1;
	f2 = (float *) p2;

	return ( *f1 > *f2 ) ? 1  :
	       ( *f2 < *f1 ) ? -1 : 0;
}

// this macro is some serious va args abuse
#define bprintf( bf, fmt, ... )		bf->len += snprintf( bf->buf + bf->len, bf->sz - bf->len, "%s" fmt " %ld\n", prfx, ## __VA_ARGS__, ts )


void stats_report_one( DHASH *d, ST_THR *cfg, time_t ts, IOBUF **buf )
{
	int i, j, tind, thr, med;
	float sum, *vals = NULL;
	PTLIST *list, *p;
	char *prfx, *ul;
	IOBUF *b;

	list = d->proc.points;
	d->proc.points = NULL;

	b    = *buf;
	prfx = ctl->stats->stats->prefix;

	for( j = 0, p = list; p; p = p->next )
		j += p->count;

	if( j > 0 )
	{
		vals = (float *) allocz( j * sizeof( float ) );

		for( j = 0, p = list; p; p = p->next )
			for( i = 0; i < p->count; i++, j++ )
				vals[j] = p->vals[i];

		sum = 0;
		kahan_summation( vals, j, &sum );

		// median offset
		med = j / 2;

		qsort( vals, j, sizeof( float ), cmp_floats );

		bprintf( b, "%s.count %d",    d->path, j );
		bprintf( b, "%s.mean %f",     d->path, sum / (float) j );
		bprintf( b, "%s.upper %f",    d->path, vals[j-1] );
		bprintf( b, "%s.lower %f",    d->path, vals[0] );
		bprintf( b, "%s.median %f",   d->path, vals[med] );

		// variable thresholds
		for( i = 0; i < ctl->stats->thr_count; i++ )
		{
			// exclude lunacy
			if( !( thr = ctl->stats->thresholds[i] ) )
				continue;

			// decide on a string
			ul = ( thr < 50 ) ? "lower" : "upper";

			// find the right index into our values
			tind = ( j * thr ) / 100;

			bprintf( b, "%s.%s_%d %f", d->path, ul, thr, vals[tind] );
		}

		if( b->len > IO_BUF_HWMK )
		{
#ifdef DEBUG
			// reuse the same buffer
			b->buf[b->len] = '\0';
			printf( "%s", b->buf );
			b->len = 0;
#else
			io_buf_send( b );
#endif
			b = mem_new_buf( IO_BUF_SZ );
			*buf = b;
		}

		free( vals );
	}

	mem_free_point_list( list );
}




void stats_stats_pass( uint64_t tval, void *arg )
{
	ST_THR *c;
	DHASH *d;
	time_t t;
	IOBUF *b;
	int i;

	t = (time_t) ( tval / 1000000 );
	c = (ST_THR *) arg;
	b = mem_new_buf( IO_BUF_SZ );

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

	// sleep a bit to avoid contention
	usleep( 1000 + ( random( ) % 30011 ) );

#ifdef DEBUG
	debug( "[%02d] Stats report", c->id );
#endif

	// and report it
	for( i = 0; i < ctl->mem->hashsize; i++ )
		if( ( i % c->max ) == c->id )
			for( d = c->conf->data[i]; d; d = d->next )
				if( d->proc.points )
				{
					if( d->empty > 0 )
						d->empty = 0;

					stats_report_one( d, c, t, &b );
				}
				else if( d->empty >= 0 )
					d->empty++;

	// anything left?
	if( b->len )
	{
#ifdef DEBUG
		b->buf[b->len] = '\0';
		printf( "%s", b->buf );
		mem_free_buf( &b );
#else
		io_buf_send( b );
#endif
	}
	else
		mem_free_buf( &b );
}



void stats_adder_pass( uint64_t tval, void *arg )
{
	char *prfx;
	ST_THR *c;
	time_t ts;
	DHASH *d;
	IOBUF *b;
	int i;

	c = (ST_THR *) arg;
	b = mem_new_buf( IO_BUF_SZ );

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

	// and report it
	for( i = 0; i < ctl->mem->hashsize; i++ )
		if( ( i % c->max ) == c->id )
			for( d = c->conf->data[i]; d; d = d->next )
				if( d->proc.sum.count > 0 )
				{
					if( d->empty > 0 )
						d->empty = 0;

					bprintf( b, "%s %f", d->path, d->proc.sum.total );

					if( b->len > IO_BUF_HWMK )
					{
#ifdef DEBUG
						// reuse the same buffer
						b->buf[b->len] = '\0';
						printf( "%s", b->buf );
						b->len = 0;
#else
						io_buf_send( b );
						b = mem_new_buf( IO_BUF_SZ );
#endif
					}
				}
				else if( d->empty >= 0 )
					d->empty++;

	// any left?
	if( b->len )
	{
#ifndef DEBUG
		io_buf_send( b );
#else
		b->buf[b->len] = '\0';
		printf( "%s", b->buf );
		mem_free_buf( &b );
#endif
	}
	else
		mem_free_buf( &b );
}



#define stats_report_mtype( nm, mt )		bytes = ((uint64_t) mt->alloc_sz) * ((uint64_t) mt->total); \
											bprintf( b, "mem.%s.free %u",  nm, mt->fcount ); \
											bprintf( b, "mem.%s.alloc %u", nm, mt->total ); \
											bprintf( b, "mem.%s.kb %lu",   nm, bytes >> 10 )

// report our own pass
void stats_self_pass( uint64_t tval, void *arg )
{
	struct timeval now;
	uint64_t bytes;
	double upt;
	char *prfx;
	time_t ts;
	IOBUF *b;

	ts   = (time_t) ( tval / 1000000 );

	now.tv_sec  = ts;
	now.tv_usec = tval % 1000000;

	prfx = ctl->stats->self->prefix;
	tvdiff( now, ctl->init_time, upt );

	b = mem_new_buf( IO_BUF_SZ );

	// TODO - more stats
	bprintf( b, "uptime %.3f", upt );
	bprintf( b, "paths.stats.curr %d", ctl->stats->stats->dcurr );
	bprintf( b, "paths.stats.gc %d",   ctl->stats->stats->gc_count );
	bprintf( b, "paths.adder.curr %d", ctl->stats->adder->dcurr );
	bprintf( b, "paths.adder.gc %d",   ctl->stats->adder->gc_count );
	bprintf( b, "mem.total.kb %d", ctl->mem->curr_kb );

	// memory
	stats_report_mtype( "hosts",  ctl->mem->hosts );
	stats_report_mtype( "points", ctl->mem->points );
	stats_report_mtype( "dhash",  ctl->mem->dhash );
	stats_report_mtype( "bufs",   ctl->mem->iobufs );
	stats_report_mtype( "iolist", ctl->mem->iolist );


#ifndef DEBUG
	io_buf_send( b );
#else
	b->buf[b->len] = '\0';
	printf( "%s", b->buf );
	mem_free_buf( &b );
#endif
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
	loop_control( cf->type, cf->loopfn, c, cf->period, 1, cf->offset );

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
		notice( "Data submission for %s is disabled.", cf->type );
		return;
	}

	// throw each of the threads
	for( i = 0; i < cf->threads; i++ )
		thread_throw( &stats_loop, &(cf->ctls[i]) );

	info( "Started %s data processing loops.", cf->type );
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

	// make the control structures
	c->ctls = (ST_THR *) allocz( c->threads * sizeof( ST_THR ) );

	for( i = 0; i < c->threads; i++ )
	{
		t = &(c->ctls[i]);

		t->conf   = c;
		t->id     = i;
		t->max    = c->threads;

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
	s->stats->type    = "stats";
	s->stats->enable  = 1;

	s->adder          = (ST_CFG *) allocz( sizeof( ST_CFG ) );
	s->adder->threads = DEFAULT_ADDER_THREADS;
	s->adder->loopfn  = &stats_adder_pass;
	s->adder->period  = DEFAULT_STATS_MSEC;
	s->adder->prefix  = stats_prefix( DEFAULT_ADDER_PREFIX );
	s->adder->type    = "adder";
	s->adder->enable  = 1;

	s->self           = (ST_CFG *) allocz( sizeof( ST_CFG ) );
	s->self->threads  = 1;
	s->self->loopfn   = &stats_self_pass;
	s->self->period   = DEFAULT_STATS_MSEC;
	s->self->prefix   = stats_prefix( DEFAULT_SELF_PREFIX );
	s->self->type     = "self";
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

