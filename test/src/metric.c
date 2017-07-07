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


const struct metric_types_data metric_types[METRIC_TYPE_MAX] =
{
	{
		.name    = "track_mean",
		.updater = &metric_update_track_mean,
		.setup   = &metric_setup_track_mean,
		.type    = METRIC_TYPE_TRACK_MEAN,
	},
	{
		.name    = "floor_up",
		.updater = &metric_update_floor_up,
		.setup   = NULL,
		.type    = METRIC_TYPE_FLOOR_UP,
	},
	{
		.name    = "sometimes_track",
		.updater = &metric_update_sometimes_track,
		.setup   = &metric_setup_track_mean,
		.type    = METRIC_TYPE_SMTS_TRACK,
	}
};



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
	if( m->d3 <= metrand( ) )
		metric_update_track_mean( m );
}





void metric_update( int64_t tval, void *arg )
{
	METRIC *m = (METRIC *) arg;
	char mbuf[32];
	BUF *pb;
	int k, l;
	IOBUF *b;

	(*(m->ufp))( m );

	pb = m->grp->prefix;

	l = snprintf( mbuf, 32, "%0.6f\n", m->curr );
	k = l + ( ( pb ) ? pb->len : 0 ) + m->path->len + 1;

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
	if( pb )
	{
		memcpy( b->buf + b->len, pb->buf, pb->len );
		b->len += pb->len;
	}

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


int metric_setup_generic( METRIC *m )
{
	return 0;
}


int metric_setup_track_mean( METRIC *m )
{
	m->d3 = m->d2 / 2;

	return metric_setup_generic( m );
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




int metric_add_group( MGRP *g )
{
	return 0;
}


int metric_add( MGRP *g, const struct metric_types_data *t, char *str, int len )
{
	//METRIC *m;
	WORDS w;

	if( strwords( &w, str, len, ' ' ) != METRIC_FLD_MAX )
	{
		err( "Invalid config for %s metric: expected %d fields.", t->name, METRIC_FLD_MAX );
		return -1;
	}

	return 0;
}



MTRC_CTL *metric_config_defaults( void )
{
	MTRC_CTL *m = (MTRC_CTL *) allocz( sizeof( MTRC_CTL ) );

	m->max_age = METRIC_MAX_AGE;

	return m;
}


/* we organise these as follows:
 *
 * All metrics are declared in groups.  Use 'done' to end a group.
 *
 * Some metrics are in a thread by themselves, pumping out as much as
 * we want them to.  These are best for stats or gauges, but paths
 * could run this way.  These are declared by the group listing them
 * as together 0.
 *
 * Some things produce something once per interval, and we run through
 * and recalculate them each interval, then submit them.  They run in
 * one thread.  These are declared by listing the group as together 1.
 *
 * There's no reasonable limit to the groups.  Each group has one
 * configured target, which must match up with a listed target.  That
 * validation is done at start time.
 *
 * Each group has one prefix which is prepended to all paths.
 */


static MGRP __mcl_grp_tmp;
static int __mcl_grp_state = 0;

int metric_config_line( AVP *av )
{
	const struct metric_types_data *mtd;
	MGRP *ng, *g = &__mcl_grp_tmp;
	int64_t v, i;

	if( !__mcl_grp_state )
		memset( g, 0, sizeof( MGRP ) );

	if( attIs( "max_age" ) )
	{
		if( parse_number( av->val, &v, NULL ) == NUM_INVALID )
			return -1;
		ctl->metric->max_age = v;
	}
	else if( attIs( "group" ) )
	{
		if( g->name )
		{
			err( "We are already processing group %s.", g->name );
			return -1;
		}

		g->name = dup_val( );
		debug( "Started group %s.", g->name );
		__mcl_grp_state = 1;
	}
	else if( attIs( "prefix" ) )
	{
		if( !g->name )
		{
			err( "Declare a group by name first.", g->name );
			return -1;
		}

		if( g->prefix )
		{
			err( "Group %s already has prefix: %s", g->name, g->prefix->buf );
			return -1;
		}

		g->prefix = strbuf_create( av->val, av->vlen );
	}
	else if( attIs( "target" ) )
	{
		if( !g->name )
		{
			err( "Declare a group by name first.", g->name );
			return -1;
		}

		if( g->target )
		{
			err( "Group %s already has target: %s", g->name, g->tgtstr );
			return -1;
		}

		g->tgtstr = dup_val( );
	}
	else if( attIs( "interval" ) )
	{
		if( !g->name )
		{
			err( "Declare a group by name first.", g->name );
			return -1;
		}

		if( parse_number( av->val, &v, NULL ) == NUM_INVALID )
		{
			err( "Invalid reporting interval for group %s: %s", g->name, av->val );
			return -1;
		}

		g->intv = v;
	}
	else if( attIs( "together" ) )
	{
		if( !g->name )
		{
			err( "Declare a group by name first.", g->name );
			return -1;
		}

		g->as_group = config_bool( av );
	}
	else if( attIs( "done" ) )
	{
		if( !g->name )
		{
			err( "Declare a group by name first.", g->name );
			return -1;
		}
		if( !g->target )
		{
			err( "Group %s has no target configured.", g->name );
			return -1;
		}
		// prefix *is* optional

		// copy the group
		ng = (MGRP *) allocz( sizeof( MGRP ) );
		memcpy( ng, g, sizeof( MGRP ) );

		// ready for another
		__mcl_grp_state = 0;

		// add add it
		return metric_add_group( ng );
	}
	else
	{
		// we allow metric update types as keywords
		for( i = 0; i < METRIC_TYPE_MAX; i++ )
		{
			mtd = metric_types + i;

			// try the name
			if( attIs( mtd->name ) )
				return metric_add( g, mtd, av->val, av->vlen );
		}

		// didn't recognise it hmm?
		return -1;
	}

	return 0;
}
