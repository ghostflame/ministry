/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem.c - memory control, free list management                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "archivist.h"


TEL *mem_new_treel( char *str, int len )
{
	TEL *t = mtype_new( ctl->mem->treel );

	t->len = len;
	t->name = allocz( len + 1 );
	memcpy( t->name, str, len );

	tree_lock_init( t );

	return t;
}


static inline void __mem_clean_treel( TEL *t )
{
	// unhooking it was the caller's problem
	t->child  = NULL;
	t->parent = NULL;
	t->he     = NULL;

	if( t->leaf )
	 	mem_free_tleaf( &(t->leaf) );

	if( t->path )
	{
		free( t->path );
		t->path = NULL;
		t->plen = 0;
	}

	free( t->name );
	t->name = NULL;
	t->len = 0;

	t->id  = 0;

	tree_lock_dest( t );
}

void mem_free_treel( TEL **t )
{
	TEL *tp;

	tp = *t;
	*t = NULL;

	__mem_clean_treel( tp );

	mtype_free( ctl->mem->treel, tp );
}


void mem_flatten_tree( TEL *root, TEL **list, TEL **end )
{
	TEL *t;

	if( !*end )
		*list = *end = root;

	while( root->child )
	{
		t = root->child;
		root->child = t->next;

		if( t->child )
			mem_flatten_tree( t, list, end );
		else
		{
			t->next = *list;
			*list = t;
		}
	}
}


void mem_free_treel_branch( TEL *br )
{
	TEL *t, *list, *end;
	int j;

	list = end = NULL;

	mem_flatten_tree( br, &list, &end );

	for( j = 0, t = list; t; t = t->next, ++j )
		__mem_clean_treel( t );

	mtype_free_list( ctl->mem->treel, j, list, end );
}

LEAF *mem_new_tleaf( void )
{
	return (LEAF *) mtype_new( ctl->mem->tleaf );
}

void mem_free_tleaf( LEAF **l )
{
	LEAF *lp;

	lp = *l;
	*l = NULL;

	memset( lp, 0, sizeof( LEAF ) );

	mtype_free( ctl->mem->tleaf, lp );
}


QP *mem_new_qrypt( void )
{
	return (QP *) mtype_new( ctl->mem->qrypt );
}

void mem_free_qrypt( QP **q )
{
	QP *qp;

	qp = *q;
	*q = NULL;

	qp->tel = NULL;
	if( qp->fq )
		mem_free_rkqry( &(qp->fq) );

	mtype_free( ctl->mem->qrypt, qp );
}

void mem_free_qrypt_list( QP *list )
{
	RKQR *fq = NULL;
	QP *p;
	int j;

	for( j = 0, p = list; p->next; p = p->next, ++j )
	{
		if( p->fq )
		{
			p->fq->next = fq;
			fq = p->fq;
			p->fq = NULL;
		}
		p->tel = NULL;
	}

	if( p->fq )
	{
		p->fq->next = fq;
		fq = p->fq;
		p->fq = NULL;
	}
	p->tel = NULL;
	++j;

	mtype_free_list( ctl->mem->qrypt, j, list, p );

	if( fq )
		mem_free_rkqry_list( fq );
}

PTS *mem_new_ptlst( void )
{
	return mtype_new( ctl->mem->ptlst );
}

void mem_free_ptlst( PTS **p )
{
	PTS *pp;

	pp = *p;
	*p = NULL;

	pp->count = 0;

	mtype_free( ctl->mem->ptlst, pp );
}

void mem_free_ptlst_list( PTS *list )
{
	PTS *p;
	int j;

	for( j = 0, p = list; p->next; p = p->next, ++j )
		p->count = 0;

	p->count = 0;
	++j;

	mtype_free_list( ctl->mem->ptlst, j, list, p );
}


RKQR *mem_new_rkqry( void )
{
	return (RKQR *) mtype_new( ctl->mem->rkqry );
}



void mem_free_rkqry( RKQR **q )
{
	RKQR *qp;

	qp = *q;
	*q = NULL;

	if( qp->data )
		mem_free_ptser( &(qp->data) );

	mtype_free( ctl->mem->rkqry, qp );
}


void mem_free_rkqry_list( RKQR *list )
{
	PTL *ptl = NULL;
	RKQR *q;
	int j;

	for( j = 0, q = list; q->next; q = q->next, ++j )
	{
		if( q->data )
		{
			q->data->next = ptl;
			ptl = q->data;
			q->data = NULL;
		}
	}

	if( q->data )
	{
		q->data->next = ptl;
		ptl = q->data;
		q->data = NULL;
	}
	++j;

	mtype_free_list( ctl->mem->rkqry, j, list, q );
}

#define MEM_PTSER_MAX_KEEP_POINTS		3601


PTL *mem_new_ptser( int count )
{
	PTL *p = mtype_new( ctl->mem->ptser );

	if( count > MEM_PTSER_MAX_KEEP_POINTS || count > p->sz )
	{
		if( p->sz )
			free( p->points );

		p->sz = count;
		p->points = (PNT *) allocz( count * sizeof( PNT ) );
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

	mtype_free( ctl->mem->ptser, pp );
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

	mtype_free_list( ctl->mem->ptser, j, list, p );
}


MEMT_CTL *memt_config_defaults( void )
{
	MEMT_CTL *m;

	m = (MEMT_CTL *) allocz( sizeof( MEMT_CTL ) );
	m->treel = mem_type_declare( "treel", sizeof( TEL ),   MEM_ALLOCSZ_TREEL, 0, 1 );
	m->tleaf = mem_type_declare( "tleaf", sizeof( LEAF ),  MEM_ALLOCSZ_TLEAF, 0, 1 );
	m->qrypt = mem_type_declare( "qrypt", sizeof( QP ),    MEM_ALLOCSZ_QRYPT, 0, 1 );
	m->ptlst = mem_type_declare( "ptlst", sizeof( PTS ),   MEM_ALLOCSZ_PTLST, 0, 1 );
	m->rkqry = mem_type_declare( "rkqry", sizeof( RKQR ),  MEM_ALLOCSZ_RKQRY, 0, 1 );
	m->ptser = mem_type_declare( "ptser", sizeof( PTL ),   MEM_ALLOCSZ_PTSER, 0, 1 );

	return m;
}

