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
#ifdef DEBUG
			debug( "Marking path %s dead.", h->path );
#endif
			h->len = 0;
			freed++;

			// clear any points waiting on this dhash immediately
			// should prevent clashes with stats_report_stats
			if( h->type == DATA_TYPE_STATS )
			{
				lock_stats( h );
				pts = h->in.points;
				h->in.points = NULL;
				unlock_stats( h );

				if( pts )
					mem_free_point_list( pts );
			}
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


static int gc_check_counter = 0;
static int gc_check_max     = 10;


void gc_pass( int64_t tval, void *arg )
{
	DHASH *flist = NULL;

	// we only do this every so often, but we need the faster
	// loop for responsiveness to shutdown

	if( ++gc_check_counter < gc_check_max )
		return;

	gc_check_counter = 0;

	gc_one_set( ctl->stats->stats, &flist, ctl->mem->gc_thresh );
	gc_one_set( ctl->stats->adder, &flist, ctl->mem->gc_thresh );
	gc_one_set( ctl->stats->gauge, &flist, ctl->mem->gc_gg_thresh );

	if( flist )
		mem_free_dhash_list( flist );
}



void *gc_loop( void *arg )
{
	THRD *t = (THRD *) arg;

	loop_control( "gc", &gc_pass, NULL, 2987653, 0, 0 );

	free( t );
	return NULL;
}



