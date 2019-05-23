/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* selfstats.c - self reporting functions
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"



#define __srmt_ct( n, c )	\
		bprintf( t, "mem.trace.%s.%s.calls %ld", ms.name, n, c.ctr ); \
		bprintf( t, "mem.trace.%s.%s.total %ld", ms.name, n, c.sum )

void self_report_mtypes( ST_THR *t )
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



void self_report_types( ST_THR *t, ST_CFG *c )
{
	float hr = (float) c->dcurr / (float) c->hsize;

	bprintf( t, "paths.%s.curr %d",         c->name, c->dcurr );
	bprintf( t, "paths.%s.gc %lu",          c->name, lockless_fetch( &(c->gc_count) ) );
	bprintf( t, "paths.%s.gc_total %lu",    c->name, c->gc_count.count );
	bprintf( t, "paths.%s.hash_ratio %.6f", c->name, hr );
}


void self_report_http_types( HTREQ *req, ST_CFG *c )
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




void self_report_netport( ST_THR *t, NET_PORT *p, char *name, char *proto, int do_drops )
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


void self_report_nettype( ST_THR *t, NET_TYPE *n )
{
	int i, drops = 0;

	if( n->flags & NTYPE_TCP_ENABLED )
	{
		bprintf( t, "network.%s.tcp.%hu.connections %d", n->name, n->tcp->port, n->conns );
		self_report_netport( t, n->tcp, n->name, "tcp", 1 );
	}

	// without checks we cannot drop UDP packets so no stats
	if( n->flags & NTYPE_UDP_CHECKS )
		drops = 1;

	if( n->flags & NTYPE_UDP_ENABLED )
		for( i = 0; i < n->udp_count; i++ )
			self_report_netport( t, n->udp[i], n->name, "udp", drops );
}


// report our own pass
void self_stats_pass( ST_THR *t )
{
	// network stats
	self_report_nettype( t, ctl->net->stats );
	self_report_nettype( t, ctl->net->adder );
	self_report_nettype( t, ctl->net->gauge );
	self_report_nettype( t, ctl->net->compat );

	// stats types
	self_report_types( t, ctl->stats->stats );
	self_report_types( t, ctl->stats->adder );
	self_report_types( t, ctl->stats->gauge );

	// memory
	self_report_mtypes( t );

	bprintf( t, "mem.total.kb %d", mem_curr_kb( ) );
	bprintf( t, "mem.total.virt_kb %d", mem_virt_kb( ) );
	bprintf( t, "uptime %.3f", ts_diff( t->now, ctl->proc->init_time, NULL ) );
	bprintf( t, "workers.selfstats.0.self_paths %ld", t->active + 1 );
}


int self_stats_cb_stats( HTREQ *req )
{
	BUF *b = req->text;

	// make sure we have 4k free
	if( ( b->sz - b->len ) < 4096 )
		req->text = strbuf_resize( req->text, b->sz + 4096 );

	json_fldo( "statsTypes" );
	self_report_http_types( req, ctl->stats->stats );
	self_report_http_types( req, ctl->stats->adder );
	self_report_http_types( req, ctl->stats->gauge );
	json_endo( );

	return 0;
}

int self_stats_cb_metrics( HTREQ *req )
{
	BUF *b = req->text;

	strbuf_aprintf( b, "# No metrics yet.\n\n" );

	return 0;
}
