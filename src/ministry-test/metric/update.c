/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* metric/update.d - functions to update metrics (implments the models)    *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



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



void metric_group_update( int64_t tval, void *arg )
{
	MGRP *g = (MGRP *) arg;
	METRIC *m;

	if( g->upd_intv > 1000000 )
		debug( "Updating group %s.", g->name );

	for( m = g->list; m; m = m->next )
		(*(m->ufp))( m );
}




void metric_group_update_loop( THRD *t )
{
	MGRP *g = (MGRP *) t->arg;

	loop_control( "group-update", metric_group_update, g, g->upd_intv, LOOP_SYNC, 0 );
}





