#include "ministry.h"


inline int cmp_floats( const void *p1, const void *p2 )
{
	float *f1, *f2;

	f1 = (float *) p1;
	f2 = (float *) p2;

	return ( *f1 > *f2 ) ? 1  :
	       ( *f2 < *f1 ) ? -1 : 0;
}



void stats_report_one( DHASH *d, ST_THR *cfg, time_t ts )
{
	float sum, *vals = NULL;
	PTLIST *list, *p;
	int i, j, nt;
	NBUF *b;

	list = d->proc.points;
	d->proc.points = NULL;

	b = cfg->target->out;

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

		// 90th percent offset
		nt = ( j * 9 ) / 10;

		qsort( vals, j, sizeof( float ), cmp_floats );

		b->len += snprintf( b->ptr + b->len, b->sz - b->len, "%s.count %d %ld\n",    d->path, j, ts );
		b->len += snprintf( b->ptr + b->len, b->sz - b->len, "%s.mean %f %ld\n",     d->path, sum / (float) j, ts );
		b->len += snprintf( b->ptr + b->len, b->sz - b->len, "%s.upper %f %ld\n",    d->path, vals[j-1], ts );
		b->len += snprintf( b->ptr + b->len, b->sz - b->len, "%s.lower %f %ld\n",    d->path, vals[0], ts );
		b->len += snprintf( b->ptr + b->len, b->sz - b->len, "%s.upper_90 %f %ld\n", d->path, vals[nt], ts );

		if( ( b->buf + b->len ) > b->hwmk )
			net_write_data( cfg->target );

		free( vals );
	}

	mem_free_point_list( list );
}




void stats_stats_pass( void *arg )
{
	ST_THR *c;
	DHASH *d;
	time_t t;
	int i;

	t = ctl->curr_time.tv_sec;
	c = (ST_THR *) arg;

	// take the data
	for( i = 0; i < ctl->data->hsize; i++ )
		if( ( i % c->max ) == c->id )
			for( d = ctl->data->stats[i]; d; d = d->next )
			{
				lock_stats( d );

				d->proc.points = d->in.points;
				d->in.points   = NULL;

				unlock_stats( d );
			}

	// and report it
	for( i = 0; i < ctl->data->hsize; i++ )
		if( ( i % c->max ) == c->id )
			for( d = ctl->data->stats[i]; d; d = d->next )
				if( d->proc.points )
				{
					d->empty = 0;
					stats_report_one( d, c, t );
				}
				else
					d->empty++;

	// anything left?
	if( c->target->out->len )
		net_write_data( c->target );
}



void stats_adder_pass( void *arg )
{
	ST_THR *c;
	time_t t;
	DHASH *d;
	NBUF *b;
	int i;

	t = ctl->curr_time.tv_sec;
	c = (ST_THR *) arg;
	b = c->target->out;

	// take the data
	for( i = 0; i < ctl->data->hsize; i++ )
		if( ( i % c->max ) == c->id )
			for( d = ctl->data->adder[i]; d; d = d->next )
			{
				lock_adder( d );

				d->proc.total = d->in.total;
				d->in.total   = 0;

				unlock_adder( d );
			}


	// and report it
	for( i = 0; i < ctl->data->hsize; i++ )
		if( ( i % c->max ) == c->id )
			for( d = ctl->data->adder[i]; d; d = d->next )
				if( d->proc.total > 0 )
				{
					d->empty = 0;

					b->len += snprintf( b->ptr + b->len, b->sz - b->len, "%s %llu %ld\n", d->path, d->proc.total, t );

					if( ( b->buf + b->len ) > b->hwmk )
						net_write_data( c->target );
				}
				else
					d->empty++;

	// any left?
	if( b->len )
		net_write_data( c->target );
}



void *stats_loop( void *arg )
{
	ST_CFG *cf;
	ST_THR *c;
	THRD *t;

	t  = (THRD *) arg;
	c  = (ST_THR *) t->arg;
	cf = c->conf;

	// try to connect now
	c->link = net_connect( c->target );

	// and then loop round
	loop_control( cf->type, cf->loopfn, c, cf->period, 1, cf->offset );

	free( t );
	return NULL;
}



void stats_start( ST_CFG *cf )
{
	int i;

	for( i = 0; i < cf->threads; i++ )
		thread_throw( &stats_loop, &(cf->ctls[i]) );

	info( "Started %s data processing loops.", cf->type );
}




// return a copy of a prefix without a trailing .
char *stats_prefix( char *s )
{
	int len;
	
	len = strlen( s );
	if( len > 0 && s[len - 1] == '.' )
		len--;

	return str_copy( s, len );
}




void stats_init_control( ST_CFG *c, char *name )
{
	ST_THR *t;
	int i;

	// convert msec to usec
	c->period *= 1000;
	c->offset *= 1000;

	// make the control structures
	c->ctls = (ST_THR *) allocz( c->threads * sizeof( ST_THR ) );

	for( i = 0; i < c->threads; i++ )
	{
		t = &(c->ctls[i]);

		t->conf   = c;
		t->id     = i;
		t->max    = c->threads;
		t->target = net_make_sock( 0, MIN_NETBUF_SZ, name, &(ctl->stats->target) );
	}
}



void stats_init( void )
{
	STAT_CTL *s = ctl->stats;
	char name[256];

	snprintf( name, 256, "%s:%hu", s->host, s->port );

	s->target.sin_family = AF_INET;
	s->target.sin_port   = htons( s->port );
	inet_aton( s->host, &(s->target.sin_addr) );

	stats_init_control( s->stats, name );
	stats_init_control( s->adder, name );
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

	s->adder          = (ST_CFG *) allocz( sizeof( ST_CFG ) );
	s->adder->threads = DEFAULT_ADDER_THREADS;
	s->adder->loopfn  = &stats_adder_pass;
	s->adder->period  = DEFAULT_STATS_MSEC;
	s->adder->prefix  = stats_prefix( DEFAULT_ADDER_PREFIX );
	s->adder->type    = "adder";

	s->host           = strdup( DEFAULT_TARGET_HOST );
	s->port           = DEFAULT_TARGET_PORT;

	return s;
}

int stats_config_line( AVP *av )
{
	ST_CFG *sc;
	char *d;
	int t;

	// nothing without a . so far
	if( !( d = strchr( av->att, '.' ) ) )
		return -1;
	d++;

	if( !strncasecmp( av->att, "target.", 7 ) )
	{
		if( !strcasecmp( d, "host" ) )
		{
			free( ctl->stats->host );
			ctl->stats->host = strdup( av->val );
		}
		else if( !strcasecmp( d, "port" ) )
		{
			ctl->stats->port = (unsigned short) strtoul( av->val, NULL, 10 );
			if( ctl->stats->port == 0 )
				ctl->stats->port = DEFAULT_TARGET_PORT;
		}
		return 0;
	}

	if( !strncasecmp( av->att, "stats.", 6 ) )
		sc = ctl->stats->stats;
	else if( !strncasecmp( av->att, "adder.", 6 ) )
		sc = ctl->stats->adder;
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
	else
		return -1;

	return 0;
}



