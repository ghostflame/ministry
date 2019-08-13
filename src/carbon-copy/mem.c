/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem.c - memory control, free list management                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "carbon_copy.h"


RDATA *mem_new_rdata( void )
{
	RDATA *r = (RDATA *) mtype_new( ctl->mem->rdata );

	pthread_mutex_init( &(r->lock), &(ctl->proc->mtxa) );

	return r;
}

void mem_free_rdata( RDATA **r )
{
	RDATA *rd;

	rd = *r;
	*r = NULL;

	pthread_mutex_destroy( &(rd->lock) );

	memset( &rd, 0, sizeof( RDATA ) );
	mtype_free( ctl->mem->rdata, rd );
}


HBUFS *mem_new_hbufs( void )
{
	HBUFS *h = mtype_new( ctl->mem->hbufs );

	// make space for buffers
	if( !h->bufs )
		h->bufs = (IOBUF **) allocz( RELAY_MAX_TARGETS * sizeof( IOBUF * ) );

	return h;
}


void mem_free_hbufs( HBUFS **h )
{
	IOBUF *bufs = NULL;
	HBUFS *hp;
	int i;

	hp = *h;
	*h = NULL;

	for( i = 0; i < hp->bcount; i++ )
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

		for( i = 0; i < h->bcount; i++ )
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

		hc++;
	}

	mtype_free_list( ctl->mem->hbufs,  hc, hfree, hend );

	mem_free_iobuf_list( bufs );
}




MEMT_CTL *memt_config_defaults( void )
{
	MEMT_CTL *m;

	m = (MEMT_CTL *) allocz( sizeof( MEMT_CTL ) );
	m->hbufs = mem_type_declare( "hbufs", sizeof( HBUFS ), MEM_ALLOCSZ_HBUFS, 0, 1 );
	m->rdata = mem_type_declare( "rdata", sizeof( RDATA ), MEM_ALLOCSZ_RDATA, 0, 1 );

	return m;
}

