/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* io/buffers.c - handles network i/o buffers                              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


// buffers

void io_buf_decr( IOBUF *buf )
{
	int refs;

	lock_buf( buf );
	refs = --(buf->refs);
	unlock_buf( buf );

	if( refs <= 0 )
		mem_free_iobuf( &buf );
}


IOBUF *io_buf_fetch( TGT *t )
{
	IOBP *bp = NULL;
	IOBUF *b = NULL;

	lock_target( t );

	if( t->tail )
	{
		// take one off the end
		bp = t->tail;
		t->tail = bp->prev;
		t->curr--;

		// was that the last one?
		if( t->tail )
			t->tail->next = NULL;
		else
			t->head = NULL;
	}

	unlock_target( t );

	if( bp )
	{
		b = bp->buf;
		mem_free_iobp( &bp );

		//tgdebug( "Grabbed buffer %p, %d remaining", b, t->curr );
	}

	return b;
}



void __io_buf_post_one( TGT *t, IOBUF *b )
{
	IOBP *bp;

	// dump buffers on anything at max
	if( t->curr >= t->max )
	{
		tgdebug( "Hit max waiting %d.", t->max );
		io_buf_decr( b );
		return;
	}

	bp = mem_new_iobp( );
	bp->buf = b;

	lock_target( t );

	if( t->tail )
	{
		bp->next = t->head;
		t->head->prev = bp;
		t->head  = bp;
	}
	else
	{
		t->head = bp;
		t->tail = bp;
	}

	++(t->curr);
	//tgdebug( "Target now has %d bufs.", t->curr );

	unlock_target( t );
}

void io_buf_post_one( TGT *t, IOBUF *b )
{
	if( b->refs <= 0 )
		b->refs = 1;

	__io_buf_post_one( t, b );
}


// post buffer to list
void io_buf_post( TGTL *l, IOBUF *buf )
{
	TGT *p;

	if( !buf || !l )
	{
		warn( "io_buf_post: nulls: %p, %p", l, buf );
		return;
	}

	if( !buf->len )
	{
		warn( "io_buf_post: empty buffer." );
		io_buf_decr( buf );
		return;
	}

	// set the refs if told
	if( buf->refs <= 0 )
		buf->refs = l->count;

	// and send to each one
	for( p = l->targets; p; p = p->next )
		__io_buf_post_one( p, buf );
}


void io_buf_next( TGT *t )
{
	IOBUF *b;

	t->curr_off  = 0;
	t->curr_len  = 0;
	t->sock->out = NULL;

	if( !( b = io_buf_fetch( t ) ) )
		return;

	t->sock->out = b;
	t->curr_len  = b->len;
}


