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


const struct metric_types_data metric_types[METRIC_MODEL_MAX] =
{
	{
		.name    = "track_mean",
		.updater = &metric_update_track_mean,
		.model   = METRIC_MODEL_TRACK_MEAN,
	},
	{
		.name    = "track_mean_pos",
		.updater = &metric_update_track_mean_pos,
		.model   = METRIC_MODEL_TRACK_MEAN_POS,
	},
	{
		.name    = "floor_up",
		.updater = &metric_update_floor_up,
		.model   = METRIC_MODEL_FLOOR_UP,
	},
	{
		.name    = "sometimes_track",
		.updater = &metric_update_sometimes_track,
		.model   = METRIC_MODEL_SMTS_TRACK,
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
	if( m->d3 <= metrand( ) )
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

	for( i = 0; i < g->repeat; i++ )
	{
		memcpy( g->buf->buf + g->buf->len, g->wtmp, l );
		g->buf->len += l;

		// check buffer rollover
		if( g->buf->len > g->buf->hwmk )
		{
			debug( "Posting group %s buffer %p (size).", g->name, g->buf );
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







MGRP *metric_find_group_parent( char *name, int nlen )
{
	MGRP *g;

	for( g = ctl->metric->flat_list; g; g = g->next_flat )
		if( g->nlen == nlen && !memcmp( g->name, name, nlen ) )
			return g;

	return NULL;
}


int metric_add_group( MGRP *g )
{
	MTRC_CTL *c = ctl->metric;

	// keep count
	c->gcount++;
	c->mcount += g->mcount;

	// put it in the flat list for parent lookups
	g->next_flat = c->flat_list;
	c->flat_list = g;

	debug( "Added metric group %s with %ld metrics in.", g->name, g->mcount );

	// is this a top-level group?
	if( !g->parent )
	{
		g->next = c->groups;
		c->groups = g;
	}
	else
	{
		g->next = g->parent->children;
		g->parent->children = g;
	}

	return 0;
}





// we allow a leading / to indicate this is a per-second count,
// rather than an interval.  If it's not, it's in msec and 
// need to convert
int metric_get_interval( char *str, int64_t *res )
{
	int inv = 0;

	if( *str == '/' )
	{
		str++;
		inv = 1;
	}

	if( parse_number( str, res, NULL ) == NUM_INVALID )
		return -1;

	// make sure the result is in usec
	if( inv )
	{
		if( !*res )
			return -1;

		*res = MILLION / *res;
	}
	else
		*res *= 1000;

	return 0;
}


int metric_add( MGRP *g, char *str, int len )
{
	METRIC *m, nm;
	char *p;
	WORDS w;
	int i;

	if( strwords( &w, str, len, ' ' ) != METRIC_FLD_MAX )
	{
		err( "Invalid config for %s metric: expected %d fields.", g->name, METRIC_FLD_MAX );
		return -1;
	}

	if( !w.len[METRIC_FLD_PATH] )
	{
		err( "No path supplied for metric." );
		return -1;
	}

	memset( &nm, 0, sizeof( METRIC ) );

	nm.path = strbuf( METRIC_MAX_PATH );
	nm.grp  = g;
	strbuf_copy( nm.path, w.wd[METRIC_FLD_PATH], w.len[METRIC_FLD_PATH] );

	if( w.len[METRIC_FLD_MODEL] )
		for( i = 0; i < METRIC_MODEL_MAX; i++ )
			if( !strcasecmp( w.wd[METRIC_FLD_MODEL], metric_types[i].name ) )
			{
				nm.model = metric_types[i].model;
				nm.ufp   = metric_types[i].updater;
				break;
			}

	if( !nm.ufp )
	{
		err( "Unrecognised model: %s", w.wd[METRIC_FLD_MODEL] );
		return -1;
	}

	p = w.wd[METRIC_FLD_TYPE];

	if( !strcasecmp( p, "adder" ) )
		nm.type = METRIC_TYPE_ADDER;
	else if( !strcasecmp( p, "stats" ) )
		nm.type = METRIC_TYPE_STATS;
	else if( !strcasecmp( p, "gauge" ) )
		nm.type = METRIC_TYPE_GAUGE;
	else
	{
		err( "Metric type %s unrecognised.", p );
		return -1;
	}

	if( parse_number( w.wd[METRIC_FLD_D1], NULL, &(nm.d1) ) == NUM_INVALID
	 || parse_number( w.wd[METRIC_FLD_D2], NULL, &(nm.d2) ) == NUM_INVALID
	 || parse_number( w.wd[METRIC_FLD_D3], NULL, &(nm.d3) ) == NUM_INVALID
	 || parse_number( w.wd[METRIC_FLD_D4], NULL, &(nm.d4) ) == NUM_INVALID )
	{
		err( "Invalid metric parameters." );
		return -1;
	}

	// a little setup
	if( nm.type == METRIC_MODEL_TRACK_MEAN
	 || nm.type == METRIC_MODEL_SMTS_TRACK )
		nm.d3 = nm.d2 / 2;

	// copy that into a new one
	m = (METRIC *) allocz( sizeof( METRIC ) );
	memcpy( m, &nm, sizeof( METRIC ) );

	// link it
	m->next = g->list;
	g->list = m;
	g->mcount++;

	debug( "Group %s adds metric: %s", g->name, m->path->buf );

	return 0;
}



MTRC_CTL *metric_config_defaults( void )
{
	MTRC_CTL *c = (MTRC_CTL *) allocz( sizeof( MTRC_CTL ) );

	c->max_age = METRIC_MAX_AGE;

	return c;
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


static MGRP *__mcl_grp_tmp = NULL;

int metric_config_line( AVP *av )
{
	MGRP *ng, *g = __mcl_grp_tmp;
	int64_t v;

	if( attIs( "maxAge" ) )
	{
		if( parse_number( av->vptr, &v, NULL ) == NUM_INVALID )
			return -1;
		ctl->metric->max_age = v;
	}
	else if( attIs( "group" ) )
	{
		ng = (MGRP *) allocz( sizeof( MGRP ) );
		ng->name = dup_val( );
		ng->nlen = av->vlen;
		ng->prefix = strbuf( METRIC_MAX_PATH );
		ng->repeat = 1;
		ng->tgttype  = -1;
		ng->upd_intv = METRIC_DEFAULT_UPD_INTV;
		ng->rep_intv = METRIC_DEFAULT_REP_INTV;

		if( __mcl_grp_tmp )
			ng->parent = __mcl_grp_tmp;

		__mcl_grp_tmp = ng;

		debug( "Started group %s.", ng->name, ng->parent );
	}
	else if( attIs( "prefix" ) )
	{
		if( !g || !g->name )
		{
			err( "Declare a group by name first.", g->name );
			return -1;
		}

		if( g->prefix->len )
		{
			err( "Group %s already has prefix: %s", g->name, g->prefix->buf );
			return -1;
		}

		strbuf_copy( g->prefix, av->vptr, av->vlen );
	}
	else if( attIs( "parent" ) )
	{
		if( !( g->parent = metric_find_group_parent( av->vptr, av->vlen ) ) )
		{
			err( "Parent %s not found for group %s", av->vptr, g->name );
			return -1;
		}
	}
	else if( attIs( "target" ) )
	{
		if( !g || !g->name )
		{
			err( "Declare a group by name first.", g->name );
			return -1;
		}

		if( g->target )
		{
			err( "Group %s already has target: %s", g->name, g->target->name );
			return -1;
		}

		if( valIs( "adder" ) )
			g->tgttype = METRIC_TYPE_ADDER;
		else if( valIs( "stats" ) )
			g->tgttype = METRIC_TYPE_STATS;
		else if( valIs( "gauge" ) )
			g->tgttype = METRIC_TYPE_GAUGE;
		else if( valIs( "compat" ) )
			g->tgttype = METRIC_TYPE_COMPAT;
		else
		{
			err( "Unrecognised target type %s.", av->vptr );
			return -1;
		}
	}
	else if( attIs( "update" ) )
	{
		if( !g || !g->name )
		{
			err( "Declare a group by name first.", g->name );
			return -1;
		}

		if( metric_get_interval( av->vptr, &(g->upd_intv) ) )
		{
			err( "Invalid update interval for group %s: %s", g->name, av->vptr );
			return -1;
		}
	}
	else if( attIs( "report" ) )
	{
		if( !g || !g->name )
		{
			err( "Declare a group by name first.", g->name );
			return -1;
		}

		if( metric_get_interval( av->vptr, &(g->rep_intv) ) )
		{
			err( "Invalid report interval for group %s: %s", g->name, av->vptr );
			return -1;
		}
	}
	else if( attIs( "repeat" ) )
	{
		if( !g || !g->name )
		{
			err( "Declare a group by name first.", g->name );
			return -1;
		}

		if( av_int( g->repeat ) == NUM_INVALID )
		{
			err( "Invalid repeat count for group %s: %s", g->name, av->vptr );
			return -1;
		}
	}
	else if( attIs( "metric" ) )
	{
		if( !g || !g->name )
		{
			err( "Declare a group by name first.", g->name );
			return -1;
		}

		return metric_add( g, av->vptr, av->vlen );
	}
	else if( attIs( "done" ) )
	{
		if( !g || !g->name )
		{
			err( "Declare a group by name first.", g->name );
			return -1;
		}

		// close config of this group
		g->closed = 1;

		// revert to the parent - or lack of
		if( g->parent && !g->parent->closed )
			__mcl_grp_tmp = g->parent;
		else
			__mcl_grp_tmp = NULL;

		return metric_add_group( g );
	}
	else
		return -1;

	return 0;
}
