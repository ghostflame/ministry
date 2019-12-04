/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* metric/init.c - metrics runtime setup functions                         *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"





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
				flagf_val( t, TGT_FLAG_ENABLED ), t->host, t->port, t->name );
	}


	// convert to nsec
	ctl->metric->max_age *= MILLION;
	metric_start_group_list( ctl->metric->groups );
}







