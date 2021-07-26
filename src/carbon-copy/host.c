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
* relay/relay.c - handles connections and processes lines                 *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "carbon_copy.h"



void relay_flush_host( RDATA *r )
{
	HBUFS *hb;
	int i;

	lock_rdata( r );

	// send all bufs that have data in
	for( hb = r->hbufs; hb; hb = hb->next )
		for( i = 0; i < hb->bcount; i++ )
			if( hb->bufs[i] && hb->bufs[i]->bf->len > 0 )
			{
				io_buf_post( hb->rule->targets[i], hb->bufs[i] );
				hb->bufs[i] = mem_new_iobuf( IO_BUF_SZ );
			}

	r->last = _proc->curr_tval;
	unlock_rdata( r );
}




/*
 *  Handle periodic flush of data
 *  Based on minimum flush time
 *
 *  Set it low for timely data without timestamps
 *  High for data with timestamps
 */
void relay_track_host( THRD *t )
{
	int64_t ns, thr, hlf, sns;
	struct timespec ts;
	RDATA *r;
	HOST *h;

	r = (RDATA *) t->arg;
	h = r->host;

	thr = ctl->relay->flush_nsec;
	hlf = thr / 2;

	while( r->running )
	{
		ns = h->last - r->last;

		if( ns > thr )
		{
			relay_flush_host( r );
			sns = thr;
		}
		else if( ns > hlf )
		{
			sns = hlf;
		}
		else
			sns = thr;

		llts( sns, ts );
		nanosleep( &ts, NULL );
	}

	if( ! RUNNING( ) )
		r->shutdown = 1;

	mem_free_rdata( &r );
}



void relay_buf_end( HOST *h )
{
	RDATA *r = (RDATA *) h->data;

	// explicitly flush the host
	relay_flush_host( r );

	// signal the tracking function to end
	// it cleans up the rdata
	r->running = 0;
}




void relay_buf_set( HOST *h )
{
	HBUFS *b, *tmp, *list;
	RDATA *rd;
	RELAY *r;
	int i;

	// even if it's not a prefix host, we still want
	// a max relay line size that is sub 64k
	if( h->lmax == 0 )
		h->lmax = 0x4000;	// 16k

	tmp = list = NULL;

	for( r = ctl->relay->rules; r; r = r->next )
	{
		b = mem_new_hbufs( );

		// set the relay function and buffers
		switch( r->type )
		{
			case RTYPE_REGEX:
				b->bufs[0] = mem_new_iobuf( IO_BUF_SZ );
				b->bcount  = 1;
				break;
			case RTYPE_HASH:
				b->bcount = r->tcount;
				for( i = 0; i < r->tcount; ++i )
					b->bufs[i] = mem_new_iobuf( IO_BUF_SZ );
				break;
		}

		b->rule = r;
		b->next = tmp;
		tmp = b;
	}

	// and get the list order correct
	while( tmp )
	{
		b = tmp;
		tmp = b->next;

		b->next = list;
		list = b;
	}

	// make a tracking thread to flush periodically
	rd = mem_new_rdata( );
	rd->host    = h;
	rd->running = 1;
	rd->hbufs   = list;

	thread_throw_named_f( &relay_track_host, rd, 0, "t_%08x:%04x",
		ntohl( h->net->peer.sin_addr.s_addr ),
		ntohs( h->net->peer.sin_port ) );

	h->data = (void *) rd;
}



