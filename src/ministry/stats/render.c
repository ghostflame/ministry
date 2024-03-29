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
* stats/render.c - stats output functions                                 *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"




// a little caution so we cannot write off the
// end of the buffer
void bprintf( ST_THR *t, char *fmt, ... )
{
	int i, total;
	va_list args;
	uint32_t l;
	TSET *s;

	// write the variable part into the thread's path buffer
	va_start( args, fmt );
	l = (uint32_t) vsnprintf( t->path->buf, t->path->sz, fmt, args );
	va_end( args );

	// check for truncation
	if( l > t->path->sz )
		l = t->path->sz;

	t->path->len = l;

	// length of prefix and path and max ts buf
	total = t->prefix->len + t->path->len + TSBUF_SZ;

	// loop through the target sets, making sure we can
	// send to them
	for( i = 0; i < ctl->tgt->set_count; ++i )
	{
		s = ctl->tgt->setarr[i];

		// are we ready for a new buffer?
		if( !buf_hasspace( t->bp[i]->bf, total ) )
		{
			io_buf_post( s->targets, t->bp[i] );

			if( !( t->bp[i] = mem_new_iobuf( IO_BUF_SZ ) ) )
			{
				fatal( "Could not allocate a new IOBUF." );
				return;
			}
		}

		// so write out the new data
		(*(s->type->wrfp))( t, t->ts[i], t->bp[i] );
	}
}



// TIMESTAMP FUNCTIONS
void stats_tsf_sec( ST_THR *t, BUF *b )
{
	strbuf_printf( b, " %ld\n", t->now.tv_sec );
}

void stats_tsf_msval( ST_THR *t, BUF *b )
{
	strbuf_printf( b, " %ld.%03ld\n", t->now.tv_sec, t->now.tv_nsec / 1000000 );
}

void stats_tsf_tval( ST_THR *t, BUF *b )
{
	strbuf_printf( b, " %ld.%06ld\n", t->now.tv_sec, t->now.tv_nsec / 1000 );
}

void stats_tsf_tspec( ST_THR *t, BUF *b )
{
	strbuf_printf( b, " %ld.%09ld\n", t->now.tv_sec, t->now.tv_nsec );
}

void stats_tsf_msec( ST_THR *t, BUF *b )
{
	strbuf_printf( b, " %ld\n", t->tval / 1000000 );
}

void stats_tsf_usec( ST_THR *t, BUF *b )
{
	strbuf_printf( b, " %ld\n", t->tval / 1000 );
}

void stats_tsf_nsec( ST_THR *t, BUF *b )
{
	strbuf_printf( b, " %ld\n", t->tval );
}


