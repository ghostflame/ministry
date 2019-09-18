/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* data/dhash.c - data hash structure handling                             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"




static const uint64_t data_path_hash_primes[8] =
{
	2909, 3001, 3083, 3187, 3259, 3343, 3517, 3581
};

/*
 * This is loosely based on DJ Bernsteins per-character hash, but with
 * significant speedup from the 32-bit in pointer cast.
 *
 * It replaces an xor based hash that showed too many collisions.
 */
__attribute__((hot)) static inline uint64_t data_path_hash( char *str, int len )
{
	register uint64_t sum = 5381;
	register int ctr, rem;
	register uint32_t *p;

	rem = len & 0x3;
	ctr = len >> 2;
	p   = (uint32_t *) str;

	/*
	 * Performance
	 *
	 * I've tried this with unrolled loops of 2, 4 and 8
	 *
	 * Loops of 8 don't happen enough - the strings aren't
	 * long enough.
	 *
	 * Loops of 4 work very well.
	 *
	 * Loops of 2 aren't enough improvement.
	 *
	 * Strangely, loops of 8 then 4 then 2 then 1 are slower
	 * than just 4 then 1, so this is pretty optimized as is.
	 */

	// a little unrolling for good measure
	while( ctr > 4 )
	{
		sum += ( sum << 5 ) + *p++;
		sum += ( sum << 5 ) + *p++;
		sum += ( sum << 5 ) + *p++;
		sum += ( sum << 5 ) + *p++;
		ctr -= 4;
	}

	// and the rest
	while( ctr-- > 0 )
		sum += ( sum << 5 ) + *p++;

	// and capture the rest
	str = (char *) p;
	while( rem-- > 0 )
		sum += *str++ * data_path_hash_primes[rem];

	return sum;
}


uint64_t data_path_hash_wrap( char *str, int len )
{
	return data_path_hash( str, len );
}



uint32_t data_get_id( ST_CFG *st )
{
	uint32_t id;

	pthread_mutex_lock( &(ctl->locks->hashstats) );

	// grab an id - these never drop
	id = ++(st->did);

	// and keep stats - gc reduces this
	++(st->dcurr);

	pthread_mutex_unlock( &(ctl->locks->hashstats) );

	return id;
}



__attribute__((hot)) static inline DHASH *data_find_path( DHASH *list, uint64_t hval, char *path, int len )
{
	register DHASH *h;

	for( h = list; h; h = h->next )
		if( h->sum == hval
		 && h->valid
		 && h->len == len
		 && !memcmp( h->path, path, len ) )
			break;

	return h;
}



DHASH *data_locate( char *path, int len, int type )
{
	uint64_t hval;
	ST_CFG *c;
	DHASH *d;

	hval = data_path_hash( path, len );

	switch( type )
	{
		case DATA_TYPE_STATS:
			c = ctl->stats->stats;
			break;
		case DATA_TYPE_ADDER:
			c = ctl->stats->adder;
			break;
		case DATA_TYPE_GAUGE:
			c = ctl->stats->gauge;
			break;
		case DATA_TYPE_HISTO:
			c = ctl->stats->histo;
			break;
		default:
			// try each in turn
			if( ( d = data_locate( path, len, DATA_TYPE_STATS ) ) )
				return d;
			if( ( d = data_locate( path, len, DATA_TYPE_ADDER ) ) )
				return d;
			if( ( d = data_locate( path, len, DATA_TYPE_GAUGE ) ) )
				return d;
			if( ( d = data_locate( path, len, DATA_TYPE_HISTO ) ) )
				return d;
			return NULL;
	}

	return data_find_path( c->data[hval % c->hsize], hval, path, len );
}


__attribute__((hot)) static inline void data_get_dhash_extras( DHASH *d )
{
	ST_HIST *h = NULL;

	switch( d->type )
	{
		case DATA_TYPE_STATS:
			if( ctl->stats->mom->enabled
			 && regex_list_test( d->path, ctl->stats->mom->rgx ) == REGEX_MATCH )
			{
				//debug( "Path %s will get moments processing.", d->path );
				d->checks |= DHASH_CHECK_MOMENTS;
			}
			if( ctl->stats->mode->enabled
			 && regex_list_test( d->path, ctl->stats->mode->rgx ) == REGEX_MATCH )
			{
				//debug( "Path %s will get mode processing.", d->path );
				d->checks |= DHASH_CHECK_MODE;
			}
			break;

		case DATA_TYPE_ADDER:
			if( ctl->stats->pred->enabled
			 && regex_list_test( d->path, ctl->stats->pred->rgx ) == REGEX_MATCH )
			{
				//debug( "Path %s will get linear regression prediction.", d->path );
				d->checks |= DHASH_CHECK_PREDICT;
				d->predict = mem_new_pred( );
			}
			break;

		case DATA_TYPE_HISTO:

			// only worth it if we have more than one config
			if( ctl->stats->histcf_count > 1 )
			{
				// try the matches first
				for( h = ctl->stats->histcf; h; h = h->next )
					if( h->enabled && regex_list_test( d->path, h->rgx ) == REGEX_MATCH )
						break;
			}

			// otherwise, choose the default
			if( !h )
				h = ctl->stats->histdefl;

			// are we replacing previous?
			if( d->in.hist.counts )
			{
				free( d->in.hist.counts );
				free( d->proc.hist.counts );
			}

			d->in.hist.counts   = (int64_t *) allocz( h->bcount * sizeof( int64_t ) );
			d->proc.hist.counts = (int64_t *) allocz( h->bcount * sizeof( int64_t ) );

			d->in.hist.conf     = h;
			d->proc.hist.conf   = h;

			break;
	}
}



__attribute__((hot)) DHASH *data_get_dhash( char *path, int len, ST_CFG *c )
{
	uint64_t hval, idx;
	DHASH *d;

	hval = data_path_hash( path, len );
	idx  = hval % c->hsize;

	if( !( d = data_find_path( c->data[idx], hval, path, len ) ) )
	{
		lock_table( idx );

		if( !( d = data_find_path( c->data[idx], hval, path, len ) ) )
		{
			if( !( d = mem_new_dhash( path, len ) ) )
			{
				fatal( "Could not allocate dhash for %s.", c->name );
				return NULL;
			}

			d->sum  = hval;
			d->type = c->dtype;
			d->next = c->data[idx];
			c->data[idx] = d;

			d->valid = 1;
		}

		unlock_table( idx );

		if( !d->id )
		{
			d->id = data_get_id( c );
			data_get_dhash_extras( d );
		}
	}

	return d;
}


