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

	for( prev = NULL, h = *list; h; h = next )
	{
		next = h->next;

		if( h->sum == 0 )
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
			// flatten the checksum
			// this means searches will pass
			// over this node
			// and we will clear it out next time
#ifdef DEBUG
			debug( "Marking path %s dead.", h->path );
#endif
			h->sum = 0;
			freed++;
		}
	}

	if( lock )
		unlock_table( idx );

	return freed;
}



void gc_pass( uint64_t tval, void *arg )
{
	DHASH *flist = NULL;
	unsigned int i;
	int fs, fa, fg;

	fs = 0;
	fa = 0;
	fg = 0;

	for( i = 0; i < ctl->mem->hashsize; i++ )
	{
		fs += gc_hash_list( &(ctl->stats->stats->data[i]), &flist, i, ctl->mem->gc_thresh );
		fa += gc_hash_list( &(ctl->stats->adder->data[i]), &flist, i, ctl->mem->gc_thresh );
		fg += gc_hash_list( &(ctl->stats->gauge->data[i]), &flist, i, ctl->mem->gc_gg_thresh );
	}

	if( flist )
		mem_free_dhash_list( flist );

	if( fs || fa )
	{
		pthread_mutex_lock( &(ctl->locks->hashstats) );

		ctl->stats->stats->dcurr -= fs;
		ctl->stats->stats->gc_count += fs;

		if( ctl->stats->stats->dcurr < 0 )
		{
			warn( "Stats dcurr went < 0." );
			ctl->stats->stats->dcurr = 0;
		}

		ctl->stats->adder->dcurr -= fa;
		ctl->stats->adder->gc_count += fa;

		if( ctl->stats->adder->dcurr < 0 )
		{
			warn( "Adder dcurr went < 0." );
			ctl->stats->adder->dcurr = 0;
		}

		ctl->stats->gauge->dcurr -= fg;
		ctl->stats->gauge->gc_count += fg;

		if( ctl->stats->gauge->dcurr < 0 )
		{
			warn( "Gauge dcurr went < 0." );
			ctl->stats->gauge->dcurr = 0;
		}

		pthread_mutex_unlock( &(ctl->locks->hashstats) );
	}
}



void *gc_loop( void *arg )
{
	THRD *t = (THRD *) arg;

	loop_control( "gc", &gc_pass, NULL, 10000000, 0, 2000000 );

	free( t );
	return NULL;
}



