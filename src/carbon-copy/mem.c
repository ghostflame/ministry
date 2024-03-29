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
* mem.c - memory control, free list management                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "carbon_copy.h"


RDATA *mem_new_rdata( void )
{
	RDATA *r = (RDATA *) mtype_new( ctl->mem->rdata );

	if( !r->linit )
	{
		pthread_mutex_init( &(r->lock), &(ctl->proc->mem->mtxa) );
		r->linit = 1;
	}

	return r;
}

void mem_free_rdata( RDATA **r )
{
	RDATA *rd;
	int8_t l;

	rd = *r;
	*r = NULL;

	if( rd->shutdown )
	{
		pthread_mutex_destroy( &(rd->lock) );
		rd->linit = 0;
	}

	l = rd->linit;
	memset( rd, 0, sizeof( RDATA ) );
	rd->linit = l;

	mtype_free( ctl->mem->rdata, rd );
}


HBUFS *mem_new_hbufs( void )
{
	HBUFS *h = mtype_new( ctl->mem->hbufs );

	// make space for buffers
	if( !h->bufs )
		h->bufs = (IOBUF **) mem_perm( RELAY_MAX_TARGETS * sizeof( IOBUF * ) );

	return h;
}


void mem_free_hbufs( HBUFS **h )
{
	IOBUF *bufs = NULL;
	HBUFS *hp;
	int i;

	hp = *h;
	*h = NULL;

	for( i = 0; i < hp->bcount; ++i )
		if( hp->bufs[i] )
		{
			hp->bufs[i]->next = bufs;
			bufs = hp->bufs[i];
			hp->bufs[i] = NULL;
		}

	hp->bcount = 0;
	hp->rule   = NULL;

	mtype_free( ctl->mem->hbufs, hp );

	mem_free_iobuf_list( bufs );
}


void mem_free_hbufs_list( HBUFS *list )
{
	HBUFS *h, *hend, *hfree = NULL;
	IOBUF *bufs = NULL;
	int i, hc = 0;

	hfree = NULL;
	hend  = list;

	while( list )
	{
		h = list;
		list = h->next;

		for( i = 0; i < h->bcount; ++i )
			if( h->bufs[i] )
			{
				h->bufs[i]->next = bufs;
				bufs = h->bufs[i];
				h->bufs[i] = NULL;
			}

		h->bcount = 0;
		h->rule   = NULL;

		h->next = hfree;
		hfree   = h;

		++hc;
	}

	mtype_free_list( ctl->mem->hbufs,  hc, hfree, hend );

	mem_free_iobuf_list( bufs );
}




MEMT_CTL *memt_config_defaults( void )
{
	MEMT_CTL *m;

	m = (MEMT_CTL *) mem_perm( sizeof( MEMT_CTL ) );
	m->hbufs = mem_type_declare( "hbufs", sizeof( HBUFS ), MEM_ALLOCSZ_HBUFS, 0, 1 );
	m->rdata = mem_type_declare( "rdata", sizeof( RDATA ), MEM_ALLOCSZ_RDATA, 0, 1 );

	return m;
}

