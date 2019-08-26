/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* metric/metric.c - metrics functions                                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


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
 * Track mean, positive only.  -ve set back to 0
 */
void metric_update_track_mean_pos( METRIC *m )
{
	metric_update_track_mean( m );

	if( m->curr < 0 )
		m->curr = 0;
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


/*
 * Gauge; uses track mean for occasional updates
 *
 * Variables
 * d1 - mean
 * d2 - variation
 * d3 - unused
 * d4 - probability of change per interval
 *
 * Setup:
 * d3 = d2/2
 *
 * Calls metric_update_track_mean for updates
 * decides for itself if it does anything
 */
void metric_update_sometimes_track( METRIC *m )
{
	if( m->d4 >= metrand( ) )
		metric_update_track_mean( m );
}


void metric_report( int64_t tval, METRIC *m )
{
	MGRP *g = m->grp;
	int l, i;

	lock_mgrp( m->grp );

	// create the line
	l = snprintf( g->wtmp, METRIC_WTMP_SZ, "%s%c%0.6f%s\n",
			m->path->buf, m->sep, m->curr,
			( m->tlen ) ? m->trlr : "" );

	for( i = 0; i < g->repeat; ++i )
	{
		memcpy( g->buf->buf + g->buf->len, g->wtmp, l );
		g->buf->len += l;

		// check buffer rollover
		if( g->buf->len > g->buf->hwmk )
		{
			//debug( "Posting group %s buffer %p (size).", g->name, g->buf );
			g->buf->refs = 1;
			io_buf_post_one( g->target, g->buf );
			g->buf = mem_new_iobuf( IO_BUF_SZ );
			g->buf->lifetime = ctl->metric->max_age;
			g->buf->expires  = tval + g->buf->lifetime;
		}
	}

	unlock_mgrp( m->grp );

	//debug( "Group %s buf size %d", g->name, g->buf->len );
}



void metric_update_group( int64_t tval, void *arg )
{
	MGRP *g = (MGRP *) arg;
	METRIC *m;

	if( g->upd_intv > 1000000 )
		debug( "Updating group %s.", g->name );

	for( m = g->list; m; m = m->next )
		(*(m->ufp))( m );
}

void metric_report_group( int64_t tval, void *arg )
{
	MGRP *g = (MGRP *) arg;
	METRIC *m;

	if( g->rep_intv > 1000000 )
		debug( "Reporting group %s.", g->name );

	for( m = g->list; m; m = m->next )
		metric_report( tval, m );
}


void metric_group_io( int64_t tval, void *arg )
{
	MGRP *g = (MGRP *) arg;

	lock_mgrp( g );

	if( tval > g->buf->expires )
	{
		// got something?  post it and get a new buffer
		if( g->buf->len > 0 )
		{
			debug( "Posting group %s buffer %p / %d (timer: %ld).", g->name, g->buf, g->buf->len, g->buf->expires );
			g->buf->refs = 1;
			io_buf_post_one( g->target, g->buf );
			g->buf = mem_new_iobuf( IO_BUF_SZ );
			g->buf->lifetime = ctl->metric->max_age;
		}

		g->buf->expires = tval + g->buf->lifetime;
	}

	unlock_mgrp( g );
}





void metric_group_update_loop( THRD *t )
{
	MGRP *g = (MGRP *) t->arg;

	loop_control( "group-update", metric_update_group, g, g->upd_intv, LOOP_SYNC, 0 );
}

void metric_group_report_loop( THRD *t )
{
	MGRP *g = (MGRP *) t->arg;
	int64_t offset;

	// select a randomized offset
	offset = 30000 + ( (int64_t) ( 40000 * metrandM( ) ) );

	loop_control( "group-report", metric_report_group, g, g->rep_intv, LOOP_SYNC, offset );
}


void metric_group_io_loop( THRD *t )
{
	// just make sure things don't age out too badly.
	loop_control( "group-io", metric_group_io, t->arg, 10000, 0, 0 );
}



// prepend one buffer to another
int metric_add_prefix( BUF *parent, BUF *child )
{
	char tmp[METRIC_MAX_PATH];

	if( !parent->len )
	{
		warn( "Parent buffer length is 0." );
		return 0;
	}

	if( ( parent->len + child->len ) >= METRIC_MAX_PATH )
	{
		warn( "Cannot create buffer over max length." );
		return -1;
	}

	memcpy( tmp, parent->buf, parent->len );
	memcpy( tmp + parent->len, child->buf, child->len );

	child->len += parent->len;
	memcpy( child->buf, tmp, child->len );
	child->buf[child->len] = '\0';

	return 0;
}


void metric_set_target_params( METRIC *m )
{
	TGT *t = m->grp->target;

	metric_add_prefix( m->grp->prefix, m->path );

	if( t->type != METRIC_TYPE_COMPAT )
	{
		// and use ministry format
		m->sep = ' ';

		// and force a type match
		if( m->type != t->type )
		{
			//warn( "Metric %s is type %d but group is type %d - changing metric type.",
			//	m->path->buf, m->type, t->type );
			m->type = t->type;
		}
	}
	else
	{
		// right, statsd format
		m->sep = ':';

		// and set some parameters for statsd format
		switch( m->type )
		{
			case METRIC_TYPE_ADDER:
				m->trlr = "|c";
				m->tlen = 2;
				break;
			case METRIC_TYPE_STATS:
				m->trlr = "|ms";
				m->tlen = 3;
				break;
			case METRIC_TYPE_GAUGE:
				m->trlr = "|g";
				m->tlen = 2;
				break;
		}
	}
}



void metric_init_group( MGRP *g )
{
	METRIC *m;

	// get ourselves a buffer
	g->buf = mem_new_iobuf( IO_BUF_SZ );
	g->buf->lifetime = ctl->metric->max_age;

	pthread_mutex_init( &(g->lock), &(ctl->proc->mtxa) );

	// do we need to merge parents?
	if( g->parent )
		metric_add_prefix( g->parent->prefix, g->prefix );

	// inherit intervals if we don't have one
	if( !g->upd_intv )
		g->upd_intv = ( g->parent ) ? g->parent->upd_intv : METRIC_DEFAULT_UPD_INTV;
	// and copy update into report if we don't have anything
	if( !g->rep_intv )
		g->rep_intv = ( g->parent ) ? g->parent->rep_intv : g->upd_intv;

	// do we have a type?
	if( g->tgttype >= 0 )
		g->target = ctl->tgt->targets[g->tgttype];

	// default to adder
	if( !g->target )
		g->target = ( g->parent ) ? g->parent->target : ctl->tgt->targets[METRIC_TYPE_ADDER];

	// we're done if there's no metrics on this one
	if( !g->mcount )
	{
		//debug( "No metrics to start in group %s.", g->name );
		return;
	}

	// do some setup on each metric
	for( m = g->list; m; m = m->next )
		metric_set_target_params( m );

	// make our write buffer
	g->wtmp = (char *) allocz( METRIC_WTMP_SZ );

	// and start up the target - we get a new one
	targets_start_one( &(g->target) );
	if( !g->target )
	{
		notice( "Group %s disabled - it's target is not configured.", g->name );
		return;
	}

	// start up the group
	debug( "Starting group %s with %d metrics @ interval %ld.", g->name, g->mcount, g->rep_intv );
	thread_throw_named_f( metric_group_update_loop, g, 0, "met_%s", g->name );
	thread_throw_named_f( metric_group_report_loop, g, 0, "rep_%s", g->name );

	// and a loop to handle io max_age
	thread_throw_named_f( metric_group_io_loop, g, 0, "iol_%s", g->name );
}



void metric_start_group_list( MGRP *list )
{
	MGRP *g;

	// work out config based on our metrics list
	for( g = list; g; g = g->next )
	{
		metric_init_group( g );

		// depth first
		if( g->children )
			metric_start_group_list( g->children );
	}
}



void metric_start_all( void )
{
	TGTL *l;
	TGT *t;

	for( l = target_list_all( ); l; l = l->next )
	{
		notice( "Target dump : List : %2d / %s", l->count, l->name );
		for( t = l->targets; t; t = t->next )
			notice( "Target dump :      : %d / %s / %hu / %s",
				t->enabled, t->host, t->port, t->name );
	}


	// convert to nsec
	ctl->metric->max_age *= MILLION;
	metric_start_group_list( ctl->metric->groups );
}







