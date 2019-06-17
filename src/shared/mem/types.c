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

		j++;
	}

	mtype_free_list( _mem->iobufs, j, freed, end );
}


IOBP *mem_new_iobp( void )
{
	return (IOBP *) mtype_new( _mem->iobps );
}

void mem_free_iobp( IOBP **b )
{
	IOBP *p;

	p  = *b;
	*b = NULL;

	p->buf  = NULL;
	p->prev = NULL;

	mtype_free( _mem->iobps, p );
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

