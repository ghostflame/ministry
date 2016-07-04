/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* selfstats.c - self reporting functions
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"



void self_report_mtype( ST_THR *t, char *name, MTYPE *mt )
{
	uint32_t freec, alloc;
	uint64_t bytes;

	// grab some stats under lock
	// we CANNOT bprintf under lock because
	// one of the reported types is buffers
	lock_mem( mt );

	// this is a guess, as stats_sz is not definitive
	bytes = (uint64_t) mt->stats_sz * (uint64_t) mt->total;
	alloc = mt->total;
	freec = mt->fcount;

	unlock_mem( mt );

	// and report them
	bprintf( t, "mem.%s.free %u",  name, freec );
	bprintf( t, "mem.%s.alloc %u", name, alloc );
	bprintf( t, "mem.%s.kb %lu",   name, bytes / 1024 );
}


void self_report_types( ST_THR *t, ST_CFG *c )
{
	float hr;

	hr = (float) c->dcurr / (float) c->hsize;

	bprintf( t, "paths.%s.curr %d",         c->name, c->dcurr );
	bprintf( t, "paths.%s.gc %d",           c->name, c->gc_count );
	bprintf( t, "paths.%s.hash_ratio %.6f", c->name, hr );
}



uint64_t stats_lockless_fetch( LLCT *l )
{
	uint64_t diff, curr;

	// we can only read from it *once*
	// then we do maths
	curr = l->count;
	diff = curr - l->prev;

	// and set the previous
	l->prev = curr;

	return diff;
}


void self_report_dlocks( ST_THR *t, DLOCKS *d )
{
	uint64_t total, diff;
	int i;

	for( total = 0, i = 0; i < d->len; i++ )
	{
		diff = stats_lockless_fetch( &(d->used[i]) );
		total += diff;

		bprintf( t, "locks.%s.%03d %lu", d->name, i, diff );
	}

	bprintf( t, "locks.%s.total %lu", d->name, total );
}


void self_report_netport( ST_THR *t, NET_PORT *p, char *name, char *proto, int do_drops )
{
	uint64_t diff;

	diff = stats_lockless_fetch( &(p->errors)  );
	bprintf( t, "network.%s.%s.%hu.errors %lu",  name, proto, p->port, diff );

	if( do_drops )
	{
		diff = stats_lockless_fetch( &(p->drops)   );
		bprintf( t, "network.%s.%s.%hu.drops %lu",   name, proto, p->port, diff );
	}

	diff = stats_lockless_fetch( &(p->accepts) );
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
#ifdef KEEP_LOCK_STATS
	// report stats usage
	self_report_dlocks( t, ctl->locks->dstats );
	self_report_dlocks( t, ctl->locks->dadder );
	self_report_dlocks( t, ctl->locks->dgauge );
#endif

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
	self_report_mtype( t, "hosts",  ctl->mem->hosts  );
	self_report_mtype( t, "points", ctl->mem->points );
	self_report_mtype( t, "dhash",  ctl->mem->dhash  );
	self_report_mtype( t, "bufs",   ctl->mem->iobufs );
	self_report_mtype( t, "iolist", ctl->mem->iolist );

	bprintf( t, "mem.total.kb %d", ctl->mem->curr_kb );
	bprintf( t, "uptime %.3f", ts_diff( t->now, ctl->init_time, NULL ) );
	bprintf( t, "workers.selfstats.0.self_paths %ld", t->active + 1 );
}


