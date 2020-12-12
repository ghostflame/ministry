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
* selfstats.c - self reporting functions
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



#define __srmt_ct( n, c )	\
		bprintf( t, "mem.trace.%s.%s.calls %ld", ms.name, n, c.ctr ); \
		bprintf( t, "mem.trace.%s.%s.total %ld", ms.name, n, c.sum )

void stats_self_report_mtypes( ST_THR *t )
{
	MTSTAT ms;
	int16_t i;

	for( i = 0; i < MEM_TYPES_MAX; ++i )
	{
		if( mem_type_stats( i, &ms ) != 0 )
			return;

		// and report them
		bprintf( t, "mem.%s.free %u",  ms.name, ms.ctrs.fcount );
		bprintf( t, "mem.%s.alloc %u", ms.name, ms.ctrs.total );
#ifdef MTYPE_TRACING
		__srmt_ct( "alloc",  ms.ctrs.all );
		__srmt_ct( "freed",  ms.ctrs.fre );
		__srmt_ct( "preall", ms.ctrs.pre );
		__srmt_ct( "refill", ms.ctrs.ref );
		bprintf( t, "mem.trace.%s.unfreed %ld",  ms.name, ( ms.ctrs.all.sum - ms.ctrs.fre.sum ) );
#endif
		bprintf( t, "mem.%s.kb %lu",   ms.name, ms.bytes / 1024 );
	}
}

#undef __srmt_ct



void stats_self_report_types( ST_THR *t, ST_CFG *c )
{
	float hr = (float) c->dcurr / (float) c->hsize;

	bprintf( t, "paths.%s.curr %d",           c->name, c->dcurr );
	bprintf( t, "paths.%s.gc %lu",            c->name, lockless_fetch( &(c->gc_count) ) );
	bprintf( t, "paths.%s.gc_total %lu",      c->name, c->gc_count.count );
	bprintf( t, "paths.%s.creates %lu",       c->name, lockless_fetch( &(c->creates) ) );
	bprintf( t, "paths.%s.creates_total %lu", c->name, c->creates.count );
	bprintf( t, "paths.%s.hash_ratio %.6f",   c->name, hr );
}





void stats_self_report_netport( ST_THR *t, NET_PORT *p, char *name, char *proto, int do_drops )
{
	uint64_t diff;

	diff = lockless_fetch( &(p->errors)  );
	bprintf( t, "network.%s.%s.%hu.errors %lu",  name, proto, p->port, diff );

	if( do_drops )
	{
		diff = lockless_fetch( &(p->drops)   );
		bprintf( t, "network.%s.%s.%hu.drops %lu",   name, proto, p->port, diff );
	}

	diff = lockless_fetch( &(p->accepts) );
	bprintf( t, "network.%s.%s.%hu.accepts %lu", name, proto, p->port, diff );
}


void stats_self_report_nettype( ST_THR *t, NET_TYPE *n )
{
	int i, drops = 0;

	if( n->flags & NTYPE_TCP_ENABLED )
	{
		bprintf( t, "network.%s.tcp.%hu.connections %d", n->name, n->tcp->port, n->conns );
		stats_self_report_netport( t, n->tcp, n->name, "tcp", 1 );
	}

	// without checks we cannot drop UDP packets so no stats
	if( n->flags & NTYPE_UDP_CHECKS )
		drops = 1;

	if( n->flags & NTYPE_UDP_ENABLED )
		for( i = 0; i < n->udp_count; ++i )
			stats_self_report_netport( t, n->udp[i], n->name, "udp", drops );
}


// report our own pass
void stats_self_stats_pass( ST_THR *t )
{
	// network stats
	stats_self_report_nettype( t, ctl->net->stats );
	stats_self_report_nettype( t, ctl->net->adder );
	stats_self_report_nettype( t, ctl->net->gauge );
	stats_self_report_nettype( t, ctl->net->histo );
	stats_self_report_nettype( t, ctl->net->compat );

	// stats types
	stats_self_report_types( t, ctl->stats->stats );
	stats_self_report_types( t, ctl->stats->adder );
	stats_self_report_types( t, ctl->stats->gauge );
	stats_self_report_types( t, ctl->stats->histo );

	// memory
	stats_self_report_mtypes( t );

	bprintf( t, "mem.total.kb %d", mem_curr_kb( ) );
	bprintf( t, "mem.total.virt_kb %d", mem_virt_kb( ) );
	bprintf( t, "uptime %.3f", ts_diff( t->now, ctl->proc->init_time, NULL ) );
	bprintf( t, "workers.selfstats.0.self_paths %ld", t->active + 1 );
}


// report on a single thread's performance
void stats_thread_report( ST_THR *t )
{
	int64_t p, tsteal, tstats, twait, tdone, delay, steal, wait, stats, total;
	double intvpc;

	// report some self stats?
	if( !ctl->stats->self->enable )
		return;

	stats_set_bufs( t, ctl->stats->self, 0 );

	// we have to capture it, because bprintf increments it
	p = t->active;

	bprintf( t, "%s.active %ld", t->wkrstr, t->active );
	bprintf( t, "%s.points %ld", t->wkrstr, t->points );
	bprintf( t, "%s.total %ld",  t->wkrstr, t->total  );

	if( t->conf->type == STATS_TYPE_STATS )
		bprintf( t, "%s.workspace %d", t->wkrstr, t->wkspcsz );

	if( t->conf->type == STATS_TYPE_STATS
	 || t->conf->type == STATS_TYPE_HISTO )
		bprintf( t, "%s.highest %d", t->wkrstr, t->highest );

	// how many predictions?
	if( t->predict )
		bprintf( t, "%s.predictions %d", t->wkrstr, t->predict );

	tsteal = tsll( t->steal );
	twait  = tsll( t->wait );
	tstats = tsll( t->stats );
	tdone  = tsll( t->done );

	delay  = tsteal - t->tval;
	stats  = tdone  - tstats;
	total  = tdone  - t->tval;

	if( twait )
	{
		steal = twait  - tsteal;
		wait  = tstats - twait;
	}
	else
	{
		steal = tstats - tsteal;
		wait  = 0;
	}

	bprintf( t, "%s.delay %ld", t->wkrstr, delay / 1000 );
	bprintf( t, "%s.steal %ld", t->wkrstr, steal / 1000 );
	bprintf( t, "%s.stats %ld", t->wkrstr, stats / 1000 );
	bprintf( t, "%s.usec %ld",  t->wkrstr, total / 1000 );

	if( wait )
		bprintf( t, "%s.wait %ld",  t->wkrstr, wait  / 1000 );

	// calculate percentage of interval
	intvpc  = (double) ( total / 10 );
	intvpc /= (double) t->conf->period;
	t->percent = intvpc;
	bprintf( t, "%s.interval_usage %.3f", t->wkrstr, intvpc );

	// and report our own paths
	bprintf( t, "%s.self_paths %ld", t->wkrstr, t->active - p + 1 );
}



void stats_self_report_http_types( json_object *js, ST_CFG *c )
{
	float hr = (float) c->dcurr / (float) c->hsize;
	json_object *jt, *jc;;

	jt = json_object_new_object( );
	jc = json_object_new_object( );

	json_insert( jt, "curr",      double, c->dcurr );
	json_insert( jt, "hashRatio", double, hr );

	json_insert( jc, "curr",      int64, lockless_fetch( &(c->gc_count) ) );
	json_insert( jc, "total",     int64, c->gc_count.count );

	json_object_object_add( jt, "gc",    jc );
	json_object_object_add( js, c->name, jt );
}


int stats_self_stats_cb_stats( json_object *jo )
{
	json_object *js;

	js = json_object_new_object( );

	stats_self_report_http_types( js, ctl->stats->stats );
	stats_self_report_http_types( js, ctl->stats->adder );
	stats_self_report_http_types( js, ctl->stats->gauge );
	stats_self_report_http_types( js, ctl->stats->histo );

	json_object_object_add( jo, "statsTypes", js );

	return 0;
}



