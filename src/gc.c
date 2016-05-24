/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* gc.c - Handles recycling data hash memory                               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"


// does not have it's own config - owned by mem.c


int gc_hash_list( DHASH **list, DHASH **flist, unsigned int idx, int thresh )
{
	int lock = 0, freed = 0;
	DHASH *h, *prev, *next;
	PTLIST *pts;

	for( prev = NULL, h = *list; h; h = next )
	{
		next = h->next;

		if( h->len == 0 )
		{
			if( !lock )
			{
				lock_table( idx );
				lock = 1;
			}

			// remove s
			if( prev )
				prev->next = next;
			else
				*list = next;

			// update the free list
			h->next = *flist;
			*flist  = h;

			// clear any waiting data points
			if( h->type == DATA_TYPE_STATS )
			{
				lock_stats( h );

				pts = h->in.points;
				h->in.points = NULL;

				unlock_stats( h );

				if( pts )
					mem_free_point_list( pts );
			}

#ifdef DEBUG
			debug( "GC on path %s", h->path );
#endif
		}
		else if( h->empty > thresh )
		{
			// flatten the path length
			// this means searches will pass
			// over this node
			// and we will clear it out next time
			h->len = 0;
			freed++;

#ifdef DEBUG
			debug( "Marked path %s dead.", h->path );
#endif
		}
	}

	if( lock )
		unlock_table( idx );

	return freed;
}


void gc_one_set( ST_CFG *c, DHASH **flist, int thresh )
{
	unsigned int i;
	int hits = 0;

	for( i = 0; i < c->hsize; i++ )
		hits += gc_hash_list( &(c->data[i]), flist, i, thresh );

	if( hits > 0 )
	{
		pthread_mutex_lock( &(ctl->locks->hashstats) );

		c->dcurr    -= hits;
		c->gc_count += hits;

		if( c->dcurr < 0 )
		{
			hits = -1;	// just a signal - prefer not to log inside the lock
			c->dcurr = 0;
		}

		pthread_mutex_unlock( &(ctl->locks->hashstats) );

		if( hits < 0 )
			warn( "Dcurr went negative for %s", c->name );
	}
}


void gc_pass( int64_t tval, void *arg )
{
	DHASH *flist = NULL;

	gc_one_set( ctl->stats->stats, &flist, ctl->mem->gc_thresh );
	gc_one_set( ctl->stats->adder, &flist, ctl->mem->gc_thresh );
	gc_one_set( ctl->stats->gauge, &flist, ctl->mem->gc_gg_thresh );

	if( flist )
		mem_free_dhash_list( flist );
}



void *gc_loop( void *arg )
{
	THRD *t = (THRD *) arg;
	int usec;

	usec = (int) ( ( (double) ctl->stats->self->period ) * 1.72143698 );

	loop_control( "gc", &gc_pass, NULL, usec, LOOP_TRIM, 0 );

	free( t );
	return NULL;
}



