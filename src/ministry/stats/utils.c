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
* stats/utils.c - helper functions for stats generation                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



void stats_set_workspace( ST_THR *t, int32_t len )
{
	int32_t sz = t->wkspcsz;
	double *b;

	// do we bother?
	if( sz > len )
		return;

	// gotta start somewhere
	if( !sz )
		sz = 2;

	// get big enough - hard limit though
	while( sz < len && sz < BILLION )
		sz *= 2;

	if( !( b = (double *) allocz( sz * sizeof( double ) ) ) )
	{
		fatal( "Unable to create new workspace buffer!" );
		return;
	}

	if( t->wkbuf1 )
		free( t->wkbuf1 );

	t->wkbuf1  = b;
	t->wkspcsz = sz;

	if( !( b = (double *) allocz( sz * sizeof( double ) ) ) )
	{
		fatal( "Unable to create new workspace secondary buffer!" );
		return;
	}

	if( t->wkbuf2 )
		free( t->wkbuf2 );

	t->wkbuf2 = b;
}




// configure the line buffer of a thread from a prefix config
void stats_set_bufs( ST_THR *t, ST_CFG *c, int64_t tval )
{
	TSET *s;
	int i;

	//debug( "Thread: %p (%s), Config: %p", t, t->wkrstr, c );

	// only set the timestamp buffer if we need to
	if( tval )
	{
		// grab the timestamp
		t->tval = tval;
		llts( tval, t->now );

		// and reset counters
		t->active  = 0;
		t->points  = 0;
		t->highest = 0;
		t->predict = 0;
	}

	// just point to the prefix buffer we want
	t->prefix = c->prefix;

	// grab new buffers
	for( i = 0; i < ctl->tgt->set_count; ++i )
	{
		s = ctl->tgt->setarr[i];

		// only set the timestamp buffers if we need to
		if( tval )
			(*(s->type->tsfp))( t, t->ts[i] );

		// grab a new buffer
		if( !t->bp[i] && !( t->bp[i] = mem_new_iobuf( IO_BUF_SZ ) ) )
		{
			fatal( "Could not allocate a new IOBUF." );
			return;
		}
	}
}

// set a prefix, make sure of a trailing .
// return a copy of a prefix with a trailing .
void stats_prefix( ST_CFG *c, char *s )
{
	int len, dot = 0;

	len = strlen( s );

	if( len > 0 )
	{
		// do we need a dot?
		if( s[len - 1] != '.' )
			dot = 1;
	}

	if( ( len + dot ) >= PREFIX_SZ )
	{
		fatal( "Prefix is larger than allowed max %d", c->prefix->sz );
		return;
	}

	// make a prefix if we've not got one
	c->prefix = strbuf_copy( c->prefix, s, len );
	if( dot )
		strbuf_add( c->prefix, ".", 1 );
}



