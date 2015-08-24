#include "ministry.h"


inline int cmp_floats( const void *p1, const void *p2 )
{
	float *f1, *f2;

	f1 = (float *) p1;
	f2 = (float *) p2;

	return ( *f1 > *f2 ) ? 1  :
	       ( *f2 < *f1 ) ? -1 : 0;
}



void stats_report_one( DSTAT *d, int which )
{
	float sum, *vals = NULL;
	PTLIST *list, *p;
	int i, j, nt;

	list = d->processing;
	d->processing = NULL;

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

		info( "[%d] %s.count %d",    which, d->path, j );
		info( "[%d] %s.mean %f",     which, d->path, sum / (float) j );
		info( "[%d] %s.upper %f",    which, d->path, vals[j - 1] );
		info( "[%d] %s.lower %f",    which, d->path, vals[0] );
		info( "[%d] %s.upper_90 %f", which, d->path, vals[nt] );

		free( vals );
	}

	mem_free_point_list( list );
}




void *stats_stats_pass( void *arg )
{
	int i, m, *p;
	DSTAT *d;
	THRD *t;

	t = (THRD *) arg;
	p = (int *) t->arg;
	m = *p;

	// take the data
	for( i = 0; i < ctl->data->hsize; i++ )
		if( ( i % ctl->stats->stats_threads ) == m )
			for( d = ctl->data->stats[i]; d; d = d->next )
			{
				lock_stats( d );

				d->processing = d->points;
				d->points     = NULL;

				unlock_stats( d );

				if( !d->processing )
					d->empty++;
			}

	// and report it
	for( i = 0; i < ctl->data->hsize; i++ )
		if( ( i % ctl->stats->stats_threads ) == m )
			for( d = ctl->data->stats[i]; d; d = d->next )
				if( d->processing )
					stats_report_one( d, m );


	free( p );
	free( t );
	return NULL;
}



void *stats_adder_pass( void *arg )
{
	int i, m, *p;
	DADD *d;
	THRD *t;

	t = (THRD *) arg;
	p = (int *) t->arg;
	m = *p;

	// take the data
	for( i = 0; i < ctl->data->hsize; i++ )
		if( ( i % ctl->stats->adder_threads ) == m )
			for( d = ctl->data->add[i]; d; d = d->next )
			{
				lock_adder( d );

				d->report = d->total;
				d->total  = 0;

				unlock_adder( d );

				if( !d->total )
					d->empty++;
			}


	// and report it
	for( i = 0; i < ctl->data->hsize; i++ )
		if( ( i % ctl->stats->adder_threads ) == m )
			for( d = ctl->data->add[i]; d; d = d->next )
				if( d->report > 0 )
					info( "[%d] %s %llu", m, d->path, d->report );


	free( p );
	free( t );
	return NULL;
}



void stats_run_stats( void )
{
	int **vals;
	int i;

	debug( "Stats run." );

	// this is to avoid problems with thread throw
	vals = (int **) allocz( ctl->stats->stats_threads * sizeof( int * ) );

	for( i = 0; i < ctl->stats->stats_threads; i++ )
	{
		vals[i]    = (int *) allocz( sizeof( int ) );
		*(vals[i]) = i;

		thread_throw( &stats_stats_pass, vals[i] );
	}

	free( vals );
}

void stats_run_adder( void )
{
	int **vals;
	int i;

	debug( "Adder run." );

	// this is to avoid problems with thread throw
	vals = (int **) allocz( ctl->stats->adder_threads * sizeof( int * ) );

	for( i = 0; i < ctl->stats->adder_threads; i++ )
	{
		vals[i]    = (int *) allocz( sizeof( int ) );
		*(vals[i]) = i;

		thread_throw( &stats_adder_pass, vals[i] );
	}

	free( vals );
}



void *stats_loop_stats( void *arg )
{
	THRD *t = (THRD *) arg;

	loop_control( "stats generation", stats_run_stats, 10000000, 1, 0 );

	free( t );
	return NULL;
}


void *stats_loop_adder( void *arg )
{
	THRD *t = (THRD *) arg;

	loop_control( "adder generation", stats_run_adder, 10000000, 1, 0 );

	free( t );
	return NULL;
}




STAT_CTL *stats_config_defaults( void )
{
	STAT_CTL *s;

	s = (STAT_CTL *) allocz( sizeof( STAT_CTL ) );

	s->stats_threads = DEFAULT_STATS_THREADS;
	s->adder_threads = DEFAULT_ADDER_THREADS;

	return s;
}

int stats_config_line( AVP *av )
{
	int t;

	if( attIs( "adder_threads" ) )
	{
		t = atoi( av->val );
		if( t > 0 )
			ctl->stats->adder_threads = t;
		else
			warn( "Adder threads must be > 0, value %d given.", t );
	}
	else if( attIs( "stats_threads" ) )
	{
		t = atoi( av->val );
		if( t > 0 )
			ctl->stats->stats_threads = t;
		else
			warn( "Stats threads must be > 0, value %d given.", t );
	}
	else
		return -1;

	return 0;
}



