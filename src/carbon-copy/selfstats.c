/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* selfstats.c - self reporting functions
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "carbon_copy.h"

// a little caution so we cannot write off the
// end of the buffer
void bprintf( ST_THR *t, char *fmt, ... )
{
	int i, total;
	va_list args;
	uint32_t l;
	TSET *s;

	// write the variable part into the thread's path buffer
	va_start( args, fmt );
	l = (uint32_t) vsnprintf( t->path->buf, t->path->sz, fmt, args );
	va_end( args );

	// check for truncation
	if( l > t->path->sz )
		l = t->path->sz;

	t->path->len = l;

	// length of prefix and path and max ts buf
	total = t->prefix->len + t->path->len + TSBUF_SZ;

	// loop through the target sets, making sure we can
	// send to them
	for( i = 0; i < ctl->tgt->set_count; i++ )
	{
		s = ctl->tgt->setarr[i];

		// are we ready for a new buffer?
		if( ( t->bp[i]->len + total ) >= IO_BUF_SZ )
		{
			target_buf_send( s, t->bp[i] );

			if( !( t->bp[i] = mem_new_iobuf( IO_BUF_SZ ) ) )
			{
				fatal( "Could not allocate a new IOBUF." );
				return;
			}
		}

		// so write out the new data
		(*(s->stype->wrfp))( t, t->ts[i], t->bp[i] );
	}
}



void self_report_mtypes( ST_THR *t )
{
	MTSTAT ms;
	int i;

	for( i = 0; i < MEM_TYPES_MAX; i++ )
	{
		mem_type_stats( i, &ms );

		if( !ms.name )
			return;

		// and report them
		bprintf( t, "mem.%s.free %u",  ms.name, ms.freec );
		bprintf( t, "mem.%s.alloc %u", ms.name, ms.alloc );
		bprintf( t, "mem.%s.kb %lu",   ms.name, ms.bytes / 1024 );
	}
}


void self_report_dlocks( ST_THR *t, DLOCKS *d )
{
	uint64_t total, diff;
	int i;

	for( total = 0, i = 0; i < d->len; i++ )
	{
		diff = lockless_fetch( &(d->used[i]) );
		total += diff;

		bprintf( t, "locks.%s.%03d %lu", d->name, i, diff );
	}

	bprintf( t, "locks.%s.total %lu", d->name, total );
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
void self_stats_pass( ST_THR *t, int64_t tval )
{
	struct timespec now;

#ifdef KEEP_LOCK_STATS
	// report stats usage
	self_report_dlocks( t, ctl->locks->dstats );
	self_report_dlocks( t, ctl->locks->dadder );
	self_report_dlocks( t, ctl->locks->dgauge );
#endif

	// network stats
	self_report_nettype( t, ctl->net->relay );

	// memory
	self_report_mtypes( t );

	bprintf( t, "mem.total.kb %d", ctl->mem->curr_kb );
	llts( tval, now );
	bprintf( t, "uptime %.3f", ts_diff( now, ctl->init_time, NULL ) );
}


