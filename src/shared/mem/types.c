/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem/types.c - built-in types of controlled memory                       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


IOBUF *mem_new_iobuf( int sz )
{
	IOBUF *b;

	b = (IOBUF *) mtype_new( _mem->iobufs );

	if( sz < 0 )
		sz = IO_BUF_SZ;

	if( b->sz < sz )
	{
		if( b->ptr )
			free( b->ptr );

		b->ptr = (char *) allocz( sz );
		b->sz  = sz;
	}

	if( b->sz == 0 )
	{
		b->buf  = NULL;
		b->hwmk = 0;
	}
	else
	{
		b->buf  = b->ptr;
		b->hwmk = ( 5 * b->sz ) / 6;
	}

	b->refs = 0;

	return b;
}



void mem_free_iobuf( IOBUF **b )
{
	IOBUF *sb;

	if( !b || !*b )
		return;

	sb = *b;
	*b = NULL;

	if( sb->sz )
		sb->ptr[0] = '\0';

	sb->buf  = NULL;
	sb->len  = 0;
	sb->refs = 0;

	sb->mtime    = 0;
	sb->expires  = 0;
	sb->lifetime = 0;

	sb->flags    = 0;

	mtype_free( _mem->iobufs, sb );
}


void mem_free_iobuf_list( IOBUF *list )
{
	IOBUF *b, *freed, *end;
	int j = 0;

	freed = NULL;
	end   = list;

	while( list )
	{
		b    = list;
		list = b->next;

		if( b->sz )
			b->ptr[0] = '\0';

		b->buf  = NULL;
		b->len  = 0;
		b->refs = 0;

		b->mtime    = 0;
		b->expires  = 0;
		b->lifetime = 0;

		b->flags = 0;

		b->next = freed;
		freed   = b;

		++j;
	}

	mtype_free_list( _mem->iobufs, j, freed, end );
}


HTREQ *mem_new_request( void )
{
	HTREQ *r = mtype_new( _mem->htreq );
	r->check = HTTP_CLS_CHECK;

	if( !r->text )
		r->text = strbuf( 4000u );

	return r;
}


void mem_free_request( HTREQ **h )
{
	HTTP_POST *p = NULL;
	BUF *b = NULL;
	HTREQ *r;

	r  = *h;
	*h = NULL;

	b = r->text;
	p = r->post;

	memset( r, 0, sizeof( HTREQ ) );

	if( p )
		memset( p, 0, sizeof( HTTP_POST ) );

	strbuf_empty( b );

	r->text = b;
	r->post = p;

	mtype_free( _mem->htreq, r );
}


HOST *mem_new_host( struct sockaddr_in *peer, uint32_t bufsz )
{
	HOST *h;

	h = (HOST *) mtype_new( _mem->hosts );

	// is this one set up?
	if( ! h->net )
	{
		h->net  = io_make_sock( bufsz, 0, peer );
		h->peer = &(h->net->peer);
	}

	// copy the peer details in
	*(h->peer) = *peer;
	h->ip      = h->peer->sin_addr.s_addr;

	// and make a name
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

	sh->lines      = 0;
	sh->invalid    = 0;
	sh->handled    = 0;
	sh->connected  = 0;
	sh->overlength = 0;
	sh->olwarn     = 0;
	sh->type       = NULL;
	sh->data       = NULL;

	sh->net->fd    = -1;
	sh->net->flags = 0;
	sh->ipn        = NULL;
	sh->ip         = 0;

	sh->peer->sin_addr.s_addr = INADDR_ANY;
	sh->peer->sin_port        = 0;

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

	mtype_free( _mem->hosts, sh );
}

TOKEN *mem_new_token( void )
{
	return (TOKEN *) mtype_new( _mem->token );
}


void mem_free_token( TOKEN **t )
{
	TOKEN *tp;

	tp = *t;
	*t = NULL;

	memset( tp, 0, sizeof( TOKEN ) );
	mtype_free( _mem->token, tp );
}

void mem_free_token_list( TOKEN *list )
{
	TOKEN *t, *freed, *end;
	int j = 0;

	freed = NULL;
	end   = list;

	while( list )
	{
		t    = list;
		list = t->next;

		memset( t, 0, sizeof( TOKEN ) );

		t->next = freed;
		freed   = t;

		++j;
	}

	mtype_free_list( _mem->token, j, freed, end );
}


MEMHG *mem_new_hanger( void *ptr )
{
	MEMHG *h = mtype_new( _mem->hanger );
	h->ptr = ptr;
	return h;
}

#define mem_clean_hanger( _h )		_h->prev = NULL; _h->ptr = NULL; _h->list = NULL

void mem_free_hanger( MEMHG **m )
{
	MEMHG *mp;

	mp = *m;
	*m = NULL;

	mem_clean_hanger( mp );
	mtype_free( _mem->hanger, mp );
}

void mem_free_hanger_list( MEMHG *list )
{
	MEMHG *m;
	int j = 0;

	for( m = list; m->next; m = m->next, ++j )
	{
		mem_clean_hanger( m );
	}

	mem_clean_hanger( m );

	mtype_free_list( _mem->hanger, j, list, m );
}

