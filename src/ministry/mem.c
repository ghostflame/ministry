/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem.c - memory control, free list management                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"




PTLIST *mem_new_points( void )
{
	return (PTLIST *) mtype_new( ctl->mem->points );
}

void mem_free_points( PTLIST **p )
{
	PTLIST *sp;

	if( !p || !*p )
		return;

	sp = *p;
	*p = NULL;

	sp->count = 0;

	mtype_free( ctl->mem->points, sp );
}

void mem_free_points_list( PTLIST *list )
{
	PTLIST *p, *freed, *end;
	int j = 0;

	freed = NULL;
	end   = list;

	while( list )
	{
		p    = list;
		list = p->next;

		p->count = 0;
		p->next  = freed;
		freed    = p;

		++j;
	}

	mtype_free_list( ctl->mem->points, j, freed, end );
}



DHASH *mem_new_dhash( const char *str, int len )
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

	// for now, base points to path
	d->base = d->path;
	d->blen = d->len;

	// we need this but it starts empty
	d->tags = "";
	d->tlen = 0;

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
	sd->blen     = 0;
	sd->in.total = 0;
	sd->in.count = 0;
	sd->type     = 0;
	sd->valid    = 0;
	sd->empty    = 0;

	if( sd->tlen )
	{
		free( sd->tags );
		free( sd->base );
	}

	sd->tlen = 0;
	sd->base = NULL;
	sd->tags = NULL;

	if( sd->in.points )
	{
		mem_free_points_list( sd->in.points );
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

	freed  = NULL;
	end    = list;
	ptfree = NULL;

	while( list )
	{
		d    = list;
		list = d->next;

		*(d->path)  = '\0';
		d->len      = 0;
		d->blen     = 0;
		d->in.total = 0;
		d->in.count = 0;
		d->type     = 0;
		d->valid    = 0;
		d->do_pass  = 0;
		d->empty    = 0;

		if( d->tlen )
		{
			free( d->tags );
			free( d->base );
		}

		d->tlen = 0;
		d->base = NULL;
		d->tags = NULL;

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

		++j;
	}

	mtype_free_list( ctl->mem->dhash, j, freed, end );

	if( ptfree )
		mem_free_points_list( ptfree );
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

	freed = NULL;
	end   = list;

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

		++j;
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

	freed = NULL;
	end   = list;

	while( list )
	{
		h = list;
		list = h->next;

		h->curr = 0;
		memset( h->points, 0, h->size * sizeof( DPT ) );

		h->next = freed;
		freed   = h;

		++j;
	}

	mtype_free_list( ctl->mem->histy, j, freed, end );
}


METRY *mem_new_metry( const char *str, int len )
{
	METRY *m = (METRY *) mtype_new( ctl->mem->metry );

	if( m->sz <= len )
	{
		free( m->metric );
		m->sz = len + 32 - ( len % 32 );
		m->metric = (char *) allocz( m->sz );
	}

	memcpy( m->metric, str, len );
	m->metric[len] = '\0';
	m->len = len;

	return m;
}

void mem_free_metry( METRY **m )
{
	METRY *mp;

	mp = *m;
	*m = NULL;

	mp->len       = 0;
	mp->metric[0] = '\0';
	mp->dtype     = NULL;

	mtype_free( ctl->mem->metry, mp );
}

void mem_free_metry_list( METRY *list )
{
	METRY *m, *freed, *end;
	int j = 0;

	freed = NULL;
	end   = list;

	while( list )
	{
		m = list;
		list = m->next;

		m->len       = 0;
		m->metric[0] = '\0';
		m->dtype     = NULL;

		m->next = freed;
		freed   = m;

		++j;
	}

	mtype_free_list( ctl->mem->metry, j, freed, end );
}




MEMT_CTL *memt_config_defaults( void )
{
	MEMT_CTL *m;

	m = (MEMT_CTL *) mem_perm( sizeof( MEMT_CTL ) );

	m->points = mem_type_declare( "points", sizeof( PTLIST ), MEM_ALLOCSZ_POINTS, 0, 1 );
	m->dhash  = mem_type_declare( "dhashs", sizeof( DHASH ),  MEM_ALLOCSZ_DHASH,  128, 1 ); // guess on path length
	m->preds  = mem_type_declare( "preds",  sizeof( PRED ),   MEM_ALLOCSZ_PREDS,  0, 1 );
	m->histy  = mem_type_declare( "histy",  sizeof( HIST ),   MEM_ALLOCSZ_HISTY,  480, 1 ); // guess on points
	m->metry  = mem_type_declare( "metry",  sizeof( METRY ),  MEM_ALLOCSZ_METRY,  64, 1 );

	return m;
}

