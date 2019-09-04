/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* metric/metric.c - metrics functions                                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"






void metric_report( int64_t tval, METRIC *m )
{
	MGRP *g = m->grp;
	int l, i;

	lock_mgrp( g );

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

	unlock_mgrp( g );

	//debug( "Group %s buf size %d", g->name, g->buf->len );
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

