/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* metric/conf.c - metrics config functions                                *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



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
	++(c->gcount);
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
		++str;
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
		for( i = 0; i < METRIC_MODEL_MAX; ++i )
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
	++(g->mcount);

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

		// special prefix: -
		// copies the group name, adds .
		if( !strcmp( g->prefix->buf, "-" ) )
		{
			strbuf_printf( g->prefix, "%s", g->name );
			if( strbuf_lastchar( g->prefix ) != '.' )
				strbuf_add( g->prefix, ".", 1 );
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
