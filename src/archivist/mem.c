/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem.c - memory control, free list management                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "archivist.h"




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


MEMT_CTL *memt_config_defaults( void )
{
	MEMT_CTL *m;

	m = (MEMT_CTL *) allocz( sizeof( MEMT_CTL ) );

	m->qrypt = mem_type_declare( "qrypt", sizeof( QP ),    MEM_ALLOCSZ_QRYPT, 0, 1 );
	m->rkqry = mem_type_declare( "rkqry", sizeof( RKQR ),  MEM_ALLOCSZ_RKQRY, 0, 1 );

	return m;
}

