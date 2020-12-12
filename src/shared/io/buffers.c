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


void __io_buf_post_one( TGT *t, IOBUF *b )
{
	// dump buffers on anything at max
	if( t->queue->count >= t->max )
	{
		tgdebug( "Hit max waiting %d.", t->max );
		io_buf_decr( b );
		return;
	}

	mem_list_add_tail( t->queue, b );
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

	if( !buf->bf->len )
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

	if( !( b = (IOBUF *) mem_list_get_head( t->queue ) ) )
		return;

	t->sock->out = b;
	t->curr_len  = b->bf->len;
}

