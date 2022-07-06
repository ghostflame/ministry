/**************************************************************************
* Copyright 2015 John Denholm                                             *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
*                                                                         *
*                                                                         *
* metric/conf.c - metrics config functions                                *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"




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
	WORDS w;

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
	{
		if( !( nm.model = metric_get_model( w.wd[METRIC_FLD_MODEL] ) ) )
			return -1;

		nm.ufp = nm.model->updater;
	}

	if( ( nm.type = metric_get_type( w.wd[METRIC_FLD_TYPE] ) ) < 0 )
		return -1;

	if( parse_number( w.wd[METRIC_FLD_D1], NULL, &(nm.d1) ) == NUM_INVALID
	 || parse_number( w.wd[METRIC_FLD_D2], NULL, &(nm.d2) ) == NUM_INVALID
	 || parse_number( w.wd[METRIC_FLD_D3], NULL, &(nm.d3) ) == NUM_INVALID
	 || parse_number( w.wd[METRIC_FLD_D4], NULL, &(nm.d4) ) == NUM_INVALID )
	{
		err( "Invalid metric parameters." );
		return -1;
	}

	// a little setup
	if( nm.model->model == METRIC_MODEL_TRACK_MEAN
	 || nm.model->model == METRIC_MODEL_SMTS_TRACK )
		nm.d3 = nm.d2 / 2;

	// copy that into a new one
	m = (METRIC *) mem_perm( sizeof( METRIC ) );
	*m = nm;

	// link it
	m->next = g->list;
	g->list = m;
	++(g->mcount);

	debug( "Group %s adds metric: %s", g->name, m->path->buf );

	return 0;
}



MTRC_CTL *metric_config_defaults( void )
{
	MTRC_CTL *c = (MTRC_CTL *) mem_perm( sizeof( MTRC_CTL ) );

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
		ng = (MGRP *) mem_perm( sizeof( MGRP ) );
		ng->name = av_copyp( av );
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

		if( ( g->tgttype = metric_get_type( av->vptr ) ) < 0 )
			return -1; 
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
