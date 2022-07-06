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
* maths/history.c - functions to control point histories                  *
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

