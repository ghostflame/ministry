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

#include "carbon_copy.h"


const char *self_ts_types[SELF_TSTYPE_MAX] =
{
	"second",
	"milli",
	"micro",
	"nano",
	"none"
};



// a little caution so we cannot write off the
// end of the buffer
void bprintf( char *fmt, ... )
{
	SST_CTL *sc = ctl->stats;
	va_list args;

	// truncate at the prefix length
	strbuf_trunc( sc->buf, sc->prlen );

	// write the variable part into the thread's path buffer
	va_start( args, fmt );
	strbuf_avprintf( sc->buf, fmt, args );
	va_end( args );

	// write the timestamp
	strbuf_add( sc->buf, sc->ts, sc->tslen );

	// relay this
	relay_simple( sc->host, sc->buf->buf, sc->buf->len );
}



void self_report_mtypes( void )
{
	MTSTAT ms;
	int i;

	for( i = 0; i < MEM_TYPES_MAX; ++i )
	{
		mem_type_stats( i, &ms );

		if( !ms.name )
			return;

		// and report them
		bprintf( "mem.%s.free %u",  ms.name, ms.ctrs.fcount );
		bprintf( "mem.%s.alloc %u", ms.name, ms.ctrs.total );
		bprintf( "mem.%s.kb %lu",   ms.name, ms.bytes / 1024 );
	}
}


void self_report_netport( NET_PORT *p, char *name, char *proto, int do_drops )
{
	uint64_t diff;

	diff = lockless_fetch( &(p->errors)  );
	bprintf( "network.%s.%s.%hu.errors %lu",  name, proto, p->port, diff );

	if( do_drops )
	{
		diff = lockless_fetch( &(p->drops)   );
		bprintf( "network.%s.%s.%hu.drops %lu",   name, proto, p->port, diff );
	}

	diff = lockless_fetch( &(p->accepts) );
	bprintf( "network.%s.%s.%hu.accepts %lu", name, proto, p->port, diff );
}


void self_report_nettype( NET_TYPE *n )
{
	int i, drops = 0;

	if( n->flags & NTYPE_TCP_ENABLED )
	{
		bprintf( "network.%s.tcp.%hu.connections %d", n->name, n->tcp->port, n->conns );
		self_report_netport( n->tcp, n->name, "tcp", 1 );
	}

	// without checks we cannot drop UDP packets so no stats
	if( n->flags & NTYPE_UDP_CHECKS )
		drops = 1;

	if( n->flags & NTYPE_UDP_ENABLED )
		for( i = 0; i < n->udp_count; ++i )
			self_report_netport( n->udp[i], n->name, "udp", drops );
}


// report our own pass
void self_stats_pass( int64_t tval, void *arg )
{
	SST_CTL *sc = ctl->stats;

	// do we need set a timestamp?
	if( sc->tsdiv )
		sc->tslen = snprintf( sc->ts, 32, " %ld", tval / sc->tsdiv );

	// network stats
	self_report_nettype( ctl->relay->net );

	// memory
	self_report_mtypes( );

	bprintf( "mem.total.kb %ld", mem_curr_kb( ) );
	bprintf( "uptime %.3f", get_uptime( ) );

	sc->host->last = tval;
}


void self_stats_loop( THRD *t )
{
	notice( "Starting the self-stats loop, interval %d sec.", ctl->stats->intv );
	loop_control( "selfstats", &self_stats_pass, NULL, ctl->stats->intv * MILLION, LOOP_TRIM|LOOP_SYNC, 0 );
}


void self_stats_init( void )
{
	SST_CTL *sc = ctl->stats;
	struct sockaddr_in sai;
	char *tmp;

	if( !sc->enabled )
		return;

	memset( &sai, 0, sizeof( struct sockaddr_in ) );

	sc->host = mem_new_host( &sai, 1 );
	sc->host->type = ctl->relay->net;
	net_set_host_parser( sc->host, 0, 0 );
	relay_buf_set( sc->host );

	sc->ts = (char *) allocz( 32 );

	switch( sc->tstype )
	{
		case SELF_TSTYPE_SEC:
			sc->tsdiv = BILLION;
			break;

		case SELF_TSTYPE_MSEC:
			sc->tsdiv = MILLION;
			break;

		case SELF_TSTYPE_USEC:
			sc->tsdiv = 1000;
			break;

		case SELF_TSTYPE_NSEC:
			sc->tsdiv = 1;
			break;
	}

	// fix the prefix
	sc->prlen = (uint32_t) strlen( sc->prefix );
	if( sc->prefix[sc->prlen - 1] != '.' )
	{
		tmp = (char *) allocz( sc->prlen + 2 );
		memcpy( tmp, sc->prefix, sc->prlen );
		tmp[sc->prlen++] = '.';
		tmp[sc->prlen]   = '\0';

		free( sc->prefix );
		sc->prefix = tmp;
	}

	sc->buf = strbuf( 4096 );
	strbuf_copy( sc->buf, sc->prefix, sc->prlen );

	thread_throw_named( &self_stats_loop, NULL, 0, "self-stats" );
}



SST_CTL *self_stats_config_defaults( void )
{
	SST_CTL *sc = (SST_CTL *) mem_perm( sizeof( SST_CTL ) );

	sc->enabled = 1;
	sc->intv    = DEFAULT_SELF_INTERVAL;
	sc->prefix  = str_copy( DEFAULT_SELF_PREFIX, 0 );

	return sc;
}


int self_stats_config_line( AVP *av )
{
	SST_CTL *sc = ctl->stats;
	int64_t v;
	int i;

	if( attIs( "enable" ) )
	{
		sc->enabled = config_bool( av );
	}
	else if( attIs( "prefix" ) )
	{
		free( sc->prefix );
		sc->prefix = av_copy( av );
	}
	else if( attIs( "timestamp" ) )
	{
		for( i = 0; i < SELF_TSTYPE_MAX; i++ )
			if( valIs( self_ts_types[i] ) )
			{
				sc->tstype = i;
				return 0;
			}

		err( "Unrecognised timestamp type '%s'.", av->vptr );
		return -1;
	}
	else if( attIs( "interval" ) || attIs( "intvSec" ) )
	{
		if( av_int( v ) == NUM_INVALID )
		{
			err( "Invalid self-stats reporting interval '%s'.", av->vptr );
			return -1;
		}
		if( v < 1 )
		{
			warn( "Minimum self-stats reporting interval is 1 sec." );
			v = 1;
		}
		sc->intv = v;
	}
	else
		return -1;

	return 0;
}

