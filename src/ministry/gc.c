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


__attribute__((hot)) int gc_hash_list( DHASH **list, DHASH **flist, PRED **plist, unsigned int idx, int thresh )
{
	int lock = 0, freed = 0;
	DHASH *h, *prev, *next;
	PTLIST *pts;

	for( prev = NULL, h = *list; h; h = next )
	{
		next = h->next;

		if( h->valid == 0 )
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

			// clear any predictor block
			if( h->predict )
			{
				h->predict->next = *plist;
				*plist = h->predict;
				h->predict = NULL;
			}
		}
		else if( h->empty > thresh )
		{
			// unset the valid flag in the first pass
			// then tidy up in the second pass
			h->valid = 0;
			++freed;
		}
	}

	if( lock )
		unlock_table( idx );

	return freed;
}


void gc_one_set( ST_CFG *c, DHASH **flist, PRED **plist, int thresh )
{
	int hits = 0;
	uint64_t i;

	for( i = 0; i < c->hsize; ++i )
		hits += gc_hash_list( &(c->data[i]), flist, plist, i, thresh );

	if( hits > 0 )
	{
		pthread_mutex_lock( &(ctl->locks->hashstats) );

		c->dcurr -= hits;

		// this is a lockless counter
		c->gc_count.count += hits;

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
	PRED *plist = NULL;

	gc_one_set( ctl->stats->stats, &flist, &plist, ctl->gc->thresh );
	gc_one_set( ctl->stats->adder, &flist, &plist, ctl->gc->thresh );
	gc_one_set( ctl->stats->gauge, &flist, &plist, ctl->gc->gg_thresh );

	if( flist )
		mem_free_dhash_list( flist );

	if( plist )
		mem_free_pred_list( plist );
}



void gc_loop( THRD *t )
{
	int64_t usec;

	// we only loop if we are gc-enabled
	// otherwise exit straight away
	if( ctl->gc->enabled ) {
		usec = (int64_t) ( ( (double) ctl->stats->self->period ) * 1.72143698 );
		loop_control( "gc", &gc_pass, NULL, usec, LOOP_TRIM, 0 );
	}
}


GC_CTL *gc_config_defaults( void )
{
	GC_CTL *g = (GC_CTL *) allocz( sizeof( GC_CTL ) );

	g->enabled        = 0;
	g->thresh         = DEFAULT_GC_THRESH;
	g->gg_thresh      = DEFAULT_GC_GG_THRESH;

	return g;
}


int gc_config_line( AVP *av )
{
	GC_CTL *g = ctl->gc;
	int64_t t;

	if( attIs( "enable" ) )
		g->enabled = config_bool( av );
	else if( attIs( "thresh" ) || attIs( "threshold" ) )
	{
		av_int( t );
		if( !t )
			t = DEFAULT_GC_THRESH;
		debug( "Garbage collection threshold set to %d stats intervals.", t );
		g->thresh = t;
	}
	else if( attIs( "gaugeThresh" ) || attIs( "gaugeThreshold" ) )
	{
		av_int( t );
		if( !t )
			t = DEFAULT_GC_GG_THRESH;
		debug( "Gauge garbage collection threshold set to %d stats intervals.", t );
		g->gg_thresh = t;
	}
	else
		return -1;

	return 0;
}


