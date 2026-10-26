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

#include "archivist.h"


QLYR *mem_new_query_layer( void )
{
	return (QLYR *) mtype_new( ctl->mem->qrlyr );
}

// two-way recursion
void __mem_flatten_query_layer_tree( QLYR *top, QLYR **list, int *count )
{
	// depth first
	if( top->child )
	{
		__mem_flatten_query_layer_tree( top->child, list, count )
	}

	// now next pointers
	if( top->next )
	{
		__mem_flatten_query_layer_tree( top->next, list, count )
	}

	// free any paths
	if( top->paths )
		mem_free_qrypt_list( top->paths );

	// remove some pieces
	memset( top, 0, sizeof( QLYR ) );

	top->next = *list;
	*list = top;
	++count;
}


// this is a tree structure
void mem_free_query_layer( QLYR **l )
{
	QLRY *lp, *list;
	int ct;

	lp = *l;
	list = NULL;
	ct = 0;

	// flatten that list
	__mem_flatten_query_layer_tree( lp, &list, &ct );

	// need to find the end of the list
	for( lp = list; lp->next; lp = lp->next );

	// now we have a flat list of layers (albeit NOT likely in ptr order)
	mtype_free_list( ctl->mem->qrlyr, ct, list, lp );
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

	qp->pcount = 0;

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

		q->pcount = 0;
	}

	if( q->data )
	{
		q->data->next = ptl;
		ptl = q->data;
		q->data = NULL;
	}

	q->pcount = 0;

	++j;

	mtype_free_list( ctl->mem->rkqry, j, list, q );
}

QARG *mem_new_qarg( void )
{
	return (QARG *) mtype_new( ctl->mem->qargs );
}

void mem_free_qarg( QARG **ap )
{
	QARG *a;

	a = *ap;
	*ap = a;

	if( a->type == 
}



MEMT_CTL *memt_config_defaults( void )
{
	MEMT_CTL *m;

	m = (MEMT_CTL *) allocz( sizeof( MEMT_CTL ) );

	m->qrypt = mem_type_declare( "qrypt", sizeof( QP ),    MEM_ALLOCSZ_QRYPT, 0, 1 );
	m->qrlyr = mem_type_declare( "qrlyr", sizeof( QLYR ),  MEM_ALLOCSZ_QRLYR, 0, 1 );
	m->rkqry = mem_type_declare( "rkqry", sizeof( RKQR ),  MEM_ALLOCSZ_RKQRY, 0, 1 );
	m->qargs = mem_type_declare( "qargs". sizeof( QARG ),  MEM_ALLOCSZ_QARGS, 0, 1 );

	return m;
}

