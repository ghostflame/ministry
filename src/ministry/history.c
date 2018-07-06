/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* history.c - functions to control point histories                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"


int history_fetch_sorted( HIST *h, HIST *into )
{
	uint16_t c;

	if( !into->size )
		into->size = h->size;

	if( into->size != h->size )
	{
		warn( "Cannot copy histories of different sizes - %hu -> %hu",
			h->size, into->size );
		return -1;
	}

	if( !into->points )
		into->points = (DPT *) allocz( into->size * sizeof( DPT ) );


	if( h->curr == 0 )
		memcpy( into->points, h->points, h->size * sizeof( DPT ) );
	else
	{
		c = h->size - h->curr;
		memcpy( into->points, h->points + h->curr, c * sizeof( DPT ) );
		memcpy( into->points + c, h->points, h->curr * sizeof( DPT ) );
	}

	into->curr = 0;

	return 0;
}



void history_add_point( HIST *h, int64_t tval, double val )
{
	h->curr = ( h->curr + 1 ) % h->size;

	h->points[h->curr].ts  = timedbl( tval );
	h->points[h->curr].val = val;
}


HIST *history_create( uint16_t size )
{
	return mem_new_history( size );
}
