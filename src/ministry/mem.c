/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem.c - memory control, free list management                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"



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

	sh->points     = 0;
	sh->invalid    = 0;
	sh->connected  = 0;
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



PTLIST *mem_new_point( void )
{
	return (PTLIST *) mtype_new( ctl->mem->points );
}

void mem_free_point( PTLIST **p )
{
	PTLIST *sp;

	if( !p || !*p )
		return;

	sp = *p;
	*p = NULL;

	sp->count    = 0;
	sp->sentinel = 0;

	mtype_free( ctl->mem->points, sp );
}

void mem_free_point_list( PTLIST *list )
{
	PTLIST *p, *freed, *end;
	int j = 0;

	freed = end = NULL;

	while( list )
	{
		p    = list;
		list = p->next;

		p->count    = 0;
		p->sentinel = 0;

		p->next  = freed;
		freed    = p;

		if( !end )
			end = p;

		j++;
	}

	mtype_free_list( ctl->mem->points, j, freed, end );
}



DHASH *mem_new_dhash( char *str, int len )
{
	DHASH *d = mtype_new( ctl->mem->dhash );

	if( len >= d->sz )
	{
		if( d->path )
			free( d->path );
		d->sz   = mem_alloc_size( len );
		d->path = (char *) allocz( d->sz );
	}

	// give that dhash a lock. dhashes love locks
	if( !d->lock )
	{
		d->lock = (dhash_lock_t *) allocz( sizeof( dhash_lock_t ) );
		linit_dhash( d );
	}

	// copy the string
	memcpy( d->path, str, len );
	d->path[len] = '\0';
	d->len = len;

	return d;
}

void mem_free_dhash( DHASH **d )
{
	DHASH *sd;

	if( !d || !*d )
		return;

	sd = *d;
	*d = NULL;

	*(sd->path)  = '\0';
	sd->len      = 0;
	sd->in.total = 0;
	sd->in.count = 0;
	sd->type     = 0;
	sd->valid    = 0;
	sd->empty    = 0;

	if( sd->in.points )
	{
		mem_free_point_list( sd->in.points );
		sd->in.points = NULL;
	}

	sd->proc.points = NULL;
	sd->proc.total  = 0;
	sd->proc.count  = 0;

	mtype_free( ctl->mem->dhash, sd );
}

void mem_free_dhash_list( DHASH *list )
{
	DHASH *d, *freed, *end;
	PTLIST *ptfree, *p;
	int j = 0;

	freed = end = NULL;
	ptfree = NULL;

	while( list )
	{
		d    = list;
		list = d->next;

		*(d->path)  = '\0';
		d->len      = 0;
		d->in.total = 0;
		d->in.count = 0;
		d->type     = 0;
		d->valid    = 0;
		d->do_pass  = 0;
		d->empty    = 0;

		if( d->in.points )
		{
			for( p = d->in.points; p->next; p = p->next );

			p->next = ptfree;
			ptfree  = d->in.points;

			d->in.points = NULL;
		}

		d->proc.points = NULL;
		d->proc.total  = 0;
		d->proc.count  = 0;

		d->next = freed;
		freed   = d;

		if( !end )
			end = d;

		j++;
	}

	mtype_free_list( ctl->mem->dhash, j, freed, end );

	if( ptfree )
		mem_free_point_list( ptfree );
}



TOKEN *mem_new_token( void )
{
	return (TOKEN *) mtype_new( ctl->mem->token );
}


void mem_free_token( TOKEN **t )
{
	TOKEN *tp;

	tp = *t;
	*t = NULL;

	memset( tp, 0, sizeof( TOKEN ) );
	mtype_free( ctl->mem->token, tp );
}

void mem_free_token_list( TOKEN *list )
{
	TOKEN *t, *freed, *end;
	int j = 0;

	freed = end = NULL;

	while( list )
	{
		t    = list;
		list = t->next;

		memset( t, 0, sizeof( TOKEN ) );

		t->next = freed;
		freed   = t;

		if( !end )
			end = t;

		j++;
	}

	mtype_free_list( ctl->mem->token, j, freed, end );
}


PRED *mem_new_pred( void )
{
	PRED *p = (PRED *) mtype_new( ctl->mem->preds );

	if( !p->hist )
		p->hist = mem_new_history( ctl->stats->pred->vsize );

	return p;
}

void mem_free_pred( PRED **p )
{
	HIST *hist;
	PRED *pp;

	pp = *p;
	*p = NULL;

	hist = pp->hist;
	memset( hist->points, 0, hist->size * sizeof( DPT ) );
	hist->curr = 0;
	memset( pp, 0, sizeof( PRED ) );
	pp->hist = hist;

	mtype_free( ctl->mem->preds, pp );
}

void mem_free_pred_list( PRED *list )
{
	PRED *p, *freed, *end;
	HIST *hist;
	int j = 0;

	freed = end = NULL;

	while( list )
	{
		p    = list;
		list = p->next;

		hist = p->hist;
		memset( hist->points, 0, hist->size * sizeof( DPT ) );
		hist->curr = 0;
		memset( p, 0, sizeof( PRED ) );
		p->hist = hist;

		p->next = freed;
		freed   = p;

		if( !end )
			end = p;

		j++;
	}

	mtype_free_list( ctl->mem->preds, j, freed, end );
}


/*
 * TODO  OK, how do we handle different history sizes?
 * Crash out for now (by returning null?)
 */
HIST *mem_new_history( uint16_t size )
{
	HIST *h = (HIST *) mtype_new( ctl->mem->histy );

	if( !h->size )
		h->size = size;

	if( h->size != size )
	{
		// crap, what can we do?
		warn( "Asked for history object with size %hu, but found size %hu on the free list.",
			size, h->size );
		mtype_free( ctl->mem->histy, h );
		return NULL;
	}

	if( !h->points )
		h->points = (DPT *) allocz( h->size * sizeof( DPT ) );

	return h;
}


void mem_free_history( HIST **h )
{
	HIST *hh;

	hh = *h;
	*h = NULL;

	memset( hh->points, 0, hh->size * sizeof( DPT ) );
	hh->curr = 0;

	mtype_free( ctl->mem->histy, hh );
}


void mem_free_history_list( HIST *list )
{
	HIST *h, *freed, *end;
	int j = 0;

	freed = end = NULL;

	while( list )
	{
		h = list;
		list = h->next;

		h->curr = 0;
		memset( h->points, 0, h->size * sizeof( DPT ) );

		h->next = freed;
		freed   = h;

		if( !end )
			end = h;

		j++;
	}

	mtype_free_list( ctl->mem->histy, j, freed, end );
}



MEMT_CTL *memt_config_defaults( void )
{
	MEMT_CTL *m;

	m = (MEMT_CTL *) allocz( sizeof( MEMT_CTL ) );

	m->hosts  = mem_type_declare( "hosts",  sizeof( HOST ),   MEM_ALLOCSZ_HOSTS,  0, 1 );
	m->points = mem_type_declare( "points", sizeof( PTLIST ), MEM_ALLOCSZ_POINTS, 0, 1 );
	m->dhash  = mem_type_declare( "dhashs", sizeof( DHASH ),  MEM_ALLOCSZ_DHASH,  128, 1 ); // guess on path length
	m->token  = mem_type_declare( "tokens", sizeof( TOKEN ),  MEM_ALLOCSZ_TOKEN,  0, 1 );
	m->preds  = mem_type_declare( "preds",  sizeof( PRED ),   MEM_ALLOCSZ_PREDS,  0, 1 );
	m->histy  = mem_type_declare( "histy",  sizeof( HIST ),   MEM_ALLOCSZ_HISTY,  480, 1 ); // guess on points

	m->gc_enabled   = 1;
	m->gc_thresh    = DEFAULT_GC_THRESH;
	m->gc_gg_thresh = DEFAULT_GC_GG_THRESH;

	return m;
}

