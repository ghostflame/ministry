/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* metric.c - networking setup and config                                  *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry_test.h"

// random number generator
#define metrand( )		( (double) random( ) / ( 1.0 * RAND_MAX ) )
#define metrandM( )		( -0.5 + metrand( ) )

/*
 * Oscillate around a mean
 *
 * Variables:
 * d1 - mean
 * d2 - variation
 * d3 - unused
 * d4 - unused
 *
 * Setup:
 * d3 = d2/2
 *
 * Halve the difference to the mean.
 * Add in a random amount 
 *
 */

void metric_update_track_mean( METRIC *m )
{
	m->curr += ( m->d2 * metrandM( ) ) - ( ( m->curr - m->d1 ) / 2 );
}


/*
 * Timer with fixed floor, random noise and occasional high values
 * Produces decent request timings
 *
 * Variables
 * d1 - floor
 * d2 - normality power (4-7 are good)
 * d3 - variation base (floor / (5-15) is good)
 * d4 - variation multiplier (2-4 is good)
 *
 * Setup:
 *
 * Generate a random value from 0-1, x
 * raise x to power of d2 to get y
 * value is floor + ( pow( d3, d4 * y ) )
 */

void metric_update_floor_up( METRIC *m )
{
	double x, y;

	x = metrand( );
	y = pow( x, m->d2 );

	m->curr = m->d1 + pow( m->d3, m->d4 * y );
}




void metric_update( int64_t tval, void *arg )
{
	METRIC *m = (METRIC *) arg;
	char mbuf[32];
	int k, l;
	IOBUF *b;

	(*(m->fp))( m );

	l = snprintf( mbuf, 32, "%0.6f\n", m->curr );
	k = l + m->grp->prefix->len + m->path->len + 1;

	// do we have to lock for this?
	if( m->use_lock )
		lock_mgrp( m->grp );

	b = m->grp->buf;

	// time to send the buffer? Or is it full?
	if( ( b->len + k ) >= IO_BUF_SZ
	 || ( ctl->curr_tval > b->expires ) )
	{
		io_post_buffer( m->grp->target->iolist, b );
		b = mem_new_buf( IO_BUF_SZ );
		m->grp->buf = b;
		b->expires = tval + ctl->metric->max_age;
	}

	// add the prefix
	memcpy( b->buf + b->len, m->grp->prefix->buf, m->grp->prefix->len );
	b->len += m->grp->prefix->len;

	// copy the path
	memcpy( b->buf + b->len, m->path->buf, m->path->len );
	b->len += m->path->len;

	// add a path
	b->buf[b->len++] = ' ';

	// and the current value (and newline)
	memcpy( b->buf + b->len, mbuf, l );
	b->len += l;

	if( m->use_lock )
		unlock_mgrp( m->grp );
}



void metric_update_group( int64_t tval, void *arg )
{
	MGRP *g = (MGRP *) arg;
	METRIC *m;

	for( m = g->list; m; m = m->next )
		metric_update( tval, m );
}



void *metric_stat_loop( void *arg )
{
	METRIC *m;
	THRD *t;

	t = (THRD *) arg;
	m = (METRIC *) t->arg;

	// work out the loop interval
	m->intv = BILLION / m->persec;

	// these ones needs lock
	m->use_lock = 1;

	loop_control( "stat", metric_update, m, m->intv, 0, 0 );

	free( t );
	return NULL;
}


void *metric_group_loop( void *arg )
{
	int64_t offset;
	MGRP *g;
	THRD *t;

	t = (THRD *) arg;
	g = (MGRP *) t->arg;

	// select a randomized offset
	offset = 30000 + ( (int64_t) ( 40000 * metrand( ) ) );

	loop_control( "group", metric_update_group, g, g->intv, LOOP_SYNC, offset );

	free( t );
	return NULL;
}






void metric_start_all( void )
{
	METRIC *m;
	MGRP *g;

	// convert to usec
	ctl->metric->max_age *= 1000;

	for( g = ctl->metric->groups; g; g = g->next )
	{
		g->intv *= 1000;
		g->buf   = mem_new_buf( IO_BUF_SZ );

		pthread_mutex_init( &(g->lock), NULL );

		// are we doing a group
		if( g->as_group )
			thread_throw( metric_group_loop, g, 0 );
		else
		{
			for( m = g->list; m; m = m->next )
				thread_throw( metric_stat_loop, m, 0 );
		}
	}
}




MTRC_CTL *metric_config_defaults( void )
{
	MTRC_CTL *m = (MTRC_CTL *) allocz( sizeof( MTRC_CTL ) );

	m->max_age = METRIC_MAX_AGE;

	return m;
}


static MGRP __mcl_grp_tmp;
static int __mcl_grp_state = 0;

int metric_config_line( AVP *av )
{
	MGRP *g = &__mcl_grp_tmp;
	int64_t v;
	char *d;

	if( !__mcl_grp_state )
		memset( g, 0, sizeof( MGRP ) );

	if( !( d = memchr( av->att, '.', av->alen ) ) )
	{
		if( attIs( "max_age" ) )
		{
			if( parse_number( av->val, &v, NULL ) == NUM_INVALID )
				return -1;
			ctl->metric->max_age = v;
		}
		else
			return -1;

		return 0;
	}


	// how is all this organised?

	if( attIs( "type"


	if( attIs( d, "path" ) )
		g->target = ctl->tgt->adder;
	else if( attIs( d, "stats" ) )
		g->target = ctl->metric->stats;
	else if( attIs( d, "gauge" ) )
		g = ctl->metric->gauge;
	else
		return -1;

	if( !strcasecmp( d, "prefix" ) )
	{
		if( g->prefix )
		{
			free( g->prefix )
	}


	return 0;
}
