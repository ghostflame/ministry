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



/*
 * Output functions
 */
void metric_line_ministry( char *rbuf, int rlen, METRIC *m )
{
	IOBUF *b = m->grp->buf;

	// add a space
	b->buf[b->len++] = ' ';

	// and the current value (and newline)
	memcpy( b->buf + b->len, rbuf, rlen );
	b->len += rlen;

	// write a timestamp for now
	b->len += snprintf( b->buf + b->len, 1000, " %ld", ctl->curr_time.tv_sec );

	// and a newline
	b->buf[b->len++] = '\n';
}


void metric_line_compat_var( char *rbuf, int rlen, char *type, int tlen, METRIC *m )
{
	IOBUF *b = m->grp->buf;

	// add a colon
	b->buf[b->len++] = ':';

	// then the value
	memcpy( b->buf + b->len, rbuf, rlen );
	b->len += rlen;

	// then the pipe
	b->buf[b->len++] = '|';

	// then the type
	memcpy( b->buf + b->len, type, tlen );
	b->len += tlen;

	// and a newline
	b->buf[b->len++] = '\n';
}

void metric_line_compat_stats( char *rbuf, int rlen, METRIC *m )
{
	metric_line_compat_var( rbuf, rlen, "ms", 2, m );
}

void metric_line_compat_adder( char *rbuf, int rlen, METRIC *m )
{
	metric_line_compat_var( rbuf, rlen, "c", 1, m );
}

void metric_line_compat_gauge( char *rbuf, int rlen, METRIC *m )
{
	metric_line_compat_var( rbuf, rlen, "g", 1, m );
}


void metric_update( int64_t tval, void *arg )
{
	METRIC *m = (METRIC *) arg;
	IOBUF *tgtbuf = NULL;
	MGRP *g = m->grp;
	char rbuf[32];
	int l;

	// update the value
	(*(m->ufp))( m );

	// render the value
	l = snprintf( rbuf, 32, "%0.6f", m->curr );

	lock_mgrp( g );

	// check buffer rollover
	if( ( g->buf->len + l + m->path->len + 100 ) > IO_BUF_SZ )
	{
		tgtbuf = g->buf;
		g->buf = mem_new_buf( IO_BUF_SZ, ctl->metric->max_age );
	}

	// copy the path - they all start with that
	memcpy( g->buf->buf + g->buf->len, m->path->buf, m->path->len );
	g->buf->len += m->path->len;

	// add to buffer
	(*(m->lfp))( rbuf, l,  m );

	unlock_mgrp( g );

	// don't do this inside the group lock to avoid deadlocks
	if( tgtbuf )
	{
		debug( "Posting group %s buffer %p (size).", g->name, tgtbuf );
		io_post_buffer( g->target->iolist, tgtbuf );
	}
}



void metric_update_group( int64_t tval, void *arg )
{
	MGRP *g = (MGRP *) arg;
	METRIC *m;

	for( m = g->list; m; m = m->next )
		metric_update( tval, m );
}


void metric_group_io( int64_t tval, void *arg )
{
	MGRP *g = (MGRP *) arg;
	IOBUF *tgtbuf = NULL;

	lock_mgrp( g );

	if( tval > g->buf->expires )
	{
		// got something?  post it and get a new buffer
		if( g->buf->len > 0 )
		{
			tgtbuf = g->buf;
			g->buf = mem_new_buf( IO_BUF_SZ, ctl->metric->max_age );
		}
		else
		{
			// reset the existing one
			g->buf->expires = tval + g->buf->lifetime;
		}
	}

	unlock_mgrp( g );

	// we don't do this inside the lock to prevent deadlocks
	if( tgtbuf )
	{
		//debug( "Posting group %s buffer %p (timer).", g->name, tgtbuf );
		io_post_buffer( g->target->iolist, tgtbuf );
	}
}





void *metric_stat_loop( void *arg )
{
	METRIC *m;
	THRD *t;

	t = (THRD *) arg;
	m = (METRIC *) t->arg;

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
	offset = 30000 + ( (int64_t) ( 40000 * metrandM( ) ) );

	loop_control( "group", metric_update_group, g, g->intv, LOOP_SYNC, offset );

	free( t );
	return NULL;
}


void *metric_group_io_loop( void *arg )
{
	THRD *t = (THRD *) arg;

	// just make sure things don't age out too badly.
	loop_control( "io", metric_group_io, t->arg, 10000, 0, 0 );

	free( t );
	return NULL;
}



// recreate the metric path with the prefix
void metric_fix_path( METRIC *m )
{
	BUF *b, *gp = m->grp->prefix;

	b = strbuf( (uint32_t) ( m->path->len + gp->len ) );

	memcpy( b->buf + b->len, gp->buf, gp->len );
	b->len += gp->len;

	memcpy( b->buf + b->len, m->path->buf, m->path->len );
	b->len += m->path->len;

	b->buf[b->len] = '\0';

	free( m->path->space );
	free( m->path );

	m->path = b;
}


void metric_set_line_function( METRIC *m )
{
	TARGET *t = m->grp->target;

	// non-compat types get forced to match
	if( t != ctl->tgt->compat )
	{
		if( m->type != t->type )
		{
			warn( "Metric %s is type %d but group is type %d - changing metric type.",
				m->path->buf, m->type, t->type );
			m->type = t->type;
		}

		m->lfp = &metric_line_ministry;
		return;
	}

	// compat we care which fn to use
	switch( m->type )
	{
		case METRIC_TYPE_ADDER:
			m->lfp = &metric_line_compat_adder;
			break;
		case METRIC_TYPE_STATS:
			m->lfp = &metric_line_compat_stats;
			break;
		case METRIC_TYPE_GAUGE:
			m->lfp = &metric_line_compat_gauge;
			break;
	}
}



void metric_start_all( void )
{
	METRIC *m;
	MGRP *g;

	// convert to nsec
	ctl->metric->max_age *= MILLION;

	for( g = ctl->metric->groups; g; g = g->next )
	{
		// get ourselves a buffer
		g->buf = mem_new_buf( IO_BUF_SZ, ctl->metric->max_age );

		pthread_mutex_init( &(g->lock), NULL );

		// do some setup
		for( m = g->list; m; m = m->next )
		{
			// set the group
			// when we created the metrics they were part
			// of the static group struct used for config
			// handling.  Now we need to point them at their
			// real group
			m->grp = g;

			metric_set_line_function( m );

			if( g->prefix )
				metric_fix_path( m );

			if( !m->intv )
				m->intv = g->intv;
		}
	}

	// and throw our threads
	for( g = ctl->metric->groups; g; g = g->next )
	{
		// are we doing a group
		if( g->as_group )
		{
			debug( "Starting group %s with interval %ld.", g->name, g->intv );
			thread_throw( metric_group_loop, g, 0 );
		}
		else
		{
			for( m = g->list; m; m = m->next )
				thread_throw( metric_stat_loop, m, 0 );
		}

		// and a loop to handle io max_age
		thread_throw( metric_group_io_loop, g, 0 );
	}
}




int metric_add_group( MGRP *g )
{
	MTRC_CTL *c = ctl->metric;
	MGRP *gp;

	for( gp = ctl->metric->groups; gp; gp = gp->next )
		if( g->nlen == gp->nlen && !memcmp( g->name, gp->name, g->nlen ) )
		{
			err( "Duplicate metric group name: %s", g->name );
			return -1;
		}

	c->gcount++;
	g->next    = c->groups;
	c->groups  = g;
	c->mcount += g->mcount;

	debug( "Added metric group %s with %ld metrics in.", g->name, g->mcount );

	return 0;
}


// we allow a leading / to indicate this is a per-second count,
// rather than an interval.  If it's not, it's in msec and we
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

	nm.path = strbuf_create( w.wd[METRIC_FLD_PATH], w.len[METRIC_FLD_PATH] );

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

	if( metric_get_interval( w.wd[METRIC_FLD_INTV], &(nm.intv) ) )
	{
		err( "Invalid interval parameter." );
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

	//debug( "Added metric %s to group %s.", m->path->buf, g->name );

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


static MGRP __mcl_grp_tmp;
static int __mcl_grp_state = 0;

int metric_config_line( AVP *av )
{
	MGRP *ng, *g = &__mcl_grp_tmp;
	int64_t v;

	if( !__mcl_grp_state )
	{
		memset( g, 0, sizeof( MGRP ) );
		g->intv = METRIC_DEFAULT_INTV;
	}

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
		g->nlen = av->vlen;
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

		// do we have a trailing .  ?
		if( g->prefix->buf[g->prefix->len - 1] != '.' )
		{
			g->prefix->buf[g->prefix->len++] = '.';
			g->prefix->buf[g->prefix->len]   = '\0';
		}
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
			err( "Group %s already has target: %s", g->name, g->target->name );
			return -1;
		}

		if( valIs( "adder" ) )
			g->target = ctl->tgt->adder;
		else if( valIs( "stats" ) )
			g->target = ctl->tgt->stats;
		else if( valIs( "gauge" ) )
			g->target = ctl->tgt->gauge;
		else if( valIs( "compat" ) )
			g->target = ctl->tgt->compat;
		else
		{
			err( "Unrecognised target %s.", av->val );
			return -1;
		}
	}
	else if( attIs( "interval" ) )
	{
		if( !g->name )
		{
			err( "Declare a group by name first.", g->name );
			return -1;
		}

		if( metric_get_interval( av->val, &(g->intv) ) )
		{
			err( "Invalid reporting interval for group %s: %s", g->name, av->val );
			return -1;
		}
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
	else if( attIs( "metric" ) )
	{
		if( !g->name )
		{
			err( "Declare a group by name first.", g->name );
			return -1;
		}

		return metric_add( g, av->val, av->vlen );
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
		return -1;

	return 0;
}
