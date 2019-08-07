/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem.c - memory control, free list management                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "carbon_copy.h"


/*
HOST *mem_new_host( struct sockaddr_in *peer, uint32_t bufsz )
{
	HOST *h;

	h = (HOST *) mtype_new( ctl->mem->hosts );

	// is this one set up?
	if( ! h->net )
	{
		h->net  = io_make_sock( bufsz, 0, peer );
		h->peer = &(h->net->peer);
	}

	// copy the peer details in
	*(h->peer) = *peer;
	h->ip      = h->peer->sin_addr.s_addr;

	// and make our name
	snprintf( h->net->name, 32, "%s:%hu", inet_ntoa( h->peer->sin_addr ),
		ntohs( h->peer->sin_port ) );

	return h;
}



void mem_free_host( HOST **h )
{
	HOST *sh;

	if( !h || !*h )
		return;

	sh = *h;
	*h = NULL;

	sh->type       = NULL;
	sh->net->fd    = -1;
	sh->net->flags = 0;
	sh->ipn        = NULL;
	sh->ip         = 0;

	sh->peer->sin_addr.s_addr = INADDR_ANY;
	sh->peer->sin_port = 0;

	if( sh->net->in )
		sh->net->in->len = 0;

	if( sh->net->out )
		sh->net->out->len = 0;

	if( sh->net->name )
		sh->net->name[0] = '\0';

	if( sh->workbuf )
	{
		sh->workbuf[0] = '\0';
		sh->plen = 0;
		sh->lmax = 0;
		sh->ltarget = sh->workbuf;
	}

	mtype_free( ctl->mem->hosts, sh );
}
*/


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
	m->hbufs = mem_type_declare( "hbufs",  sizeof( HBUFS ),  MEM_ALLOCSZ_HBUFS,  0, 1 );

	return m;
}

