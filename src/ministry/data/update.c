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
* data/update.c - update the individual data types' storage               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"



__attribute__((hot)) void data_update_histo( DHASH *d, double val, char unused )
{
	register int i;
	ST_HIST *c;
	DHIST *h;

	h = &(d->in.hist);
	c = h->conf;

	// find the right boundary
	for( i = 0; i < c->brange; ++i )
		if( val <= c->bounds[i] )
			break;

	// if we don't find one, i == c->brange
	// which is the +inf count

	lock_histo( d );

	++(h->counts[i]);
	++(d->in.count);

	unlock_histo( d ); 
}


__attribute__((hot)) void data_update_gauge( DHASH *d, double val, char op )
{
	// lock that path
	lock_gauge( d );

	// add in the data, based on the op
	// add, subtract or set
	switch( op )
	{
		case '+':
			d->in.total += val;
			break;
		case '-':
			d->in.total -= val;
			break;
		default:
			d->in.total = val;
			break;
	}

	++(d->in.count);

	// and unlock
	unlock_gauge( d );
}


// just copy the point fn - too quick to do a call
__attribute__((hot)) void data_update_adder( DHASH *d, double val, char unused )
{
	// lock that path
	lock_adder( d );

	// add in that data point
	d->in.total += val;
	++(d->in.count);

	// and unlock
	unlock_adder( d );
}


__attribute__((hot)) void data_update_stats( DHASH *d, double val, char unused )
{
	PTLIST *p;

	// lock that path
	lock_stats( d );

	// make a new one if need be
	if( !( p = d->in.points ) || p->count >= PTLIST_SIZE )
	{
		if( !( p = mem_new_points( ) ) )
		{
			fatal( "Could not allocate new point struct." );
			unlock_stats( d );
			return;
		}

		p->next = d->in.points;
		d->in.points = p;
	}

	// keep that data point
	p->vals[p->count] = val;
	++(d->in.count);
	++(p->count);

	// and unlock
	unlock_stats( d );
}



