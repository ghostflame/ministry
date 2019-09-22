/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* data/update.c - update the individual data types' storage               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"



__attribute__((hot)) void data_update_histo( DHASH *d, double val, char unused )
{
	ST_HIST *c;
	DHIST *h;
	int i;

	h = &(d->in.hist);
	c = h->conf;

	lock_histo( d );

	// find the right boundary
	for( i = 0; i < c->brange; i++ )
		if( val <= c->bounds[i] )
		{
			++(h->counts[i]);
			break;
		}

	if( i == c->brange )
		++(h->counts[c->brange]);

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
		if( !( p = mem_new_point( ) ) )
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



