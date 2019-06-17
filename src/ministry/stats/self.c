/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
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

	for( i = 0; i < MEM_TYPES_MAX; i++ )
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

	bprintf( t, "paths.%s.curr %d",         c->name, c->dcurr );
	bprintf( t, "paths.%s.gc %lu",          c->name, lockless_fetch( &(c->gc_count) ) );
	bprintf( t, "paths.%s.gc_total %lu",    c->name, c->gc_count.count );
	bprintf( t, "paths.%s.hash_ratio %.6f", c->name, hr );
}


void stats_self_report_http_types( HTREQ *req, ST_CFG *c )
{
	float hr = (float) c->dcurr / (float) c->hsize;
	BUF *b = req->text;

	json_fldo( c->name );

	json_fldi( "curr", c->dcurr );
	json_fldf6( "hashRatio", hr );

	json_fldo( "gc" );
	json_fldU( "curr", lockless_fetch( &(c->gc_count) ) );
	json_fldU( "total", c->gc_count.count );
	json_endo( );

	json_endo( );
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
		for( i = 0; i < n->udp_count; i++ )
			stats_self_report_netport( t, n->udp[i], n->name, "udp", drops );
}


// report our own pass
void stats_self_stats_pass( ST_THR *t )
{
	// network stats
	stats_self_report_nettype( t, ctl->net->stats );
	stats_self_report_nettype( t, ctl->net->adder );
	stats_self_report_nettype( t, ctl->net->gauge );
	stats_self_report_nettype( t, ctl->net->compat );

	// stats types
	stats_self_report_types( t, ctl->stats->stats );
	stats_self_report_types( t, ctl->stats->adder );
	stats_self_report_types( t, ctl->stats->gauge );

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
	{
		bprintf( t, "%s.workspace %d", t->wkrstr, t->wkspcsz );
		bprintf( t, "%s.highest %d",   t->wkrstr, t->highest );
	}

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




int stats_self_stats_cb_stats( HTREQ *req )
{
	BUF *b = req->text;

	// make sure we have 4k free
	if( ( b->sz - b->len ) < 4090 )
		strbuf_resize( req->text, b->sz + 4090 );

	json_fldo( "statsTypes" );
	stats_self_report_http_types( req, ctl->stats->stats );
	stats_self_report_http_types( req, ctl->stats->adder );
	stats_self_report_http_types( req, ctl->stats->gauge );
	json_endo( );

	return 0;
}



