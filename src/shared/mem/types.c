/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem/types.c - built-in types of controlled memory                       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


IOBUF *mem_new_iobuf( int32_t sz )
{
	IOBUF *b;

	b = (IOBUF *) mtype_new( _mem->iobufs );

	if( sz < 0 )
		sz = IO_BUF_SZ;

	if( !b->bf )
		b->bf = strbuf( sz );
	else if( b->bf->sz < (uint32_t) sz )
		b->bf = strbuf_resize( b->bf, sz );

	b->hwmk = ( 5 * b->bf->sz ) / 6;
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

	// this one is complex, making a single
	// function not much faster than the list
	// handler, so we don't replicate the logic
	sb->next = NULL;
	mem_free_iobuf_list( sb );
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

		strbuf_empty( b->bf );

		b->refs     = 0;
		b->mtime    = 0;
		b->expires  = 0;
		b->lifetime = 0;
		b->flags    = 0;
		b->hwmk     = 0;

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

	if( r->params )
		mem_free_htprm_list( r->params );

	memset( r, 0, sizeof( HTREQ ) );

	if( p )
		memset( p, 0, sizeof( HTTP_POST ) );

	strbuf_empty( b );

	r->text = b;
	r->post = p;

	mtype_free( _mem->htreq, r );
}

HTPRM *mem_new_htprm( void )
{
	return (HTPRM *) mtype_new( _mem->htprm );
}

#define mem_clean_htprm( _h )		_h->key[0] = '\0'; _h->val[0] = '\0'; _h->klen = 0; _h->vlen = 0; _h->has_val = 0


void mem_free_htprm( HTPRM **p )
{
	HTPRM *pp;

	pp = *p;
	*p = NULL;

	mem_clean_htprm( pp );

	mtype_free( _mem->htprm, pp );
}

void mem_free_htprm_list( HTPRM *list )
{
	HTPRM *p, *pn;
	int j;

	for( j = 0, p = list; p->next; p = p->next, ++j )
	{
		pn = p->next;
		mem_clean_htprm( p );
		p->next = pn;
	}

	// and do the last one
	mem_clean_htprm( p );

	mtype_free_list( _mem->htprm, j, list, p );
}


HOST *mem_new_host( struct sockaddr_in *peer, uint32_t bufsz )
{
	HOST *h;

	h = (HOST *) mtype_new( _mem->hosts );

	// is this one set up?
	if( ! h->net )
	{
		h->net  = io_make_sock( bufsz, 0, peer, 0, NULL );
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
	sh->lock_use   = 0;
	sh->ipn        = NULL;
	sh->ip         = 0;

	sh->peer->sin_addr.s_addr = INADDR_ANY;
	sh->peer->sin_port        = 0;

	if( sh->net->in )
	{
		strbuf_empty( sh->net->in->bf );
	}


	if( sh->net->out )
	{
		strbuf_empty( sh->net->out->bf );
	}

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


MEMHG *mem_new_hanger( void *ptr, MEMHL *list )
{
	MEMHG *h = mtype_new( _mem->hanger );
	h->ptr  = ptr;
	h->list = list;
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
	MEMHG *m, *end = list;
	int j = 0;

	for( m = list; m; m = m->next, ++j )
	{
		mem_clean_hanger( m );
		end = m;
	}

	mtype_free_list( _mem->hanger, j, list, end);
}


SLKMSG *mem_new_slack_msg( size_t sz )
{
	SLKMSG *m;

	m = mtype_new( _mem->slkmsg );

	if( !sz )
		sz = SLACK_MSG_BUF_SZ;

	if( !m->text )
		m->text = strbuf( sz );

	return m;
}

void mem_free_slack_msg( SLKMSG **m )
{
	SLKMSG *mp;

	mp = *m;
	*m = NULL;

	strbuf_empty( mp->text );

	if( mp->line )
	{
		free( mp->line );
		mp->line = NULL;
	}

	mtype_free( _mem->slkmsg, mp );
}

SSTE *mem_new_store( void )
{
	return (SSTE *) mtype_new( _mem->store );
}

void mem_free_store( SSTE **s )
{
	SSTE *sp;

	sp = *s;
	*s = NULL;

	if( !flagf_has( sp, STRSTORE_FLAG_FREEABLE ) )
	{
		err( "Cannot free string store entry not marked freeable." );
		return;
	}

	if( sp->len && sp->str )
		free( sp->str );

	memset( sp, 0, sizeof( SSTE ) );

	mtype_free( _mem->store, sp );
}

void mem_free_store_list( SSTE *list )
{
	SSTE *s, *end = NULL;
	int j = 0;

	for( s = list; s; s = s->next, ++j )
	{
		if( !flagf_has( s, STRSTORE_FLAG_FREEABLE ) )
		{
			err( "Cannot free string store entry without free flag." );
			// not safe to continue with the list
			return;
		}

		if( s->len && s->str )
			free( s->str );

		memset( s, 0, sizeof( SSTE ) );
		end = s;
	}

	mtype_free_list( _mem->store, j, list, end );
}



PTL *mem_new_ptser( int count )
{
	PTL *p = mtype_new( _mem->ptser );

	if( count > MEM_PTSER_MAX_KEEP_POINTS || count > p->sz )
	{
		if( p->sz )
			free( p->points );

		p->sz = count;
		p->points = allocz( count * sizeof( PNT ) );
	}

	return p;
 }

#define mem_clean_ptser( _p )			\
	if( _p->sz > MEM_PTSER_MAX_KEEP_POINTS ) \
	{ \
		free( _p->points ); \
		_p->points = NULL; \
		_p->sz = 0; \
	} \
	_p->count = 0

void mem_free_ptser( PTL **p )
{
	PTL *pp;

	pp = *p;
	*p = NULL;

	mem_clean_ptser( pp );

	mtype_free( _mem->ptser, pp );
}

void mem_free_ptser_list( PTL *list )
{
	PTL *p;
	int j;

	for( p = list, j = 0; p->next; p = p->next, ++j )
	{
		mem_clean_ptser( p );
	}

	mem_clean_ptser( p );
	++j;

	mtype_free_list( _mem->ptser, j, list, p );
}

