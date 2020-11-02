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
 *
 * UPDATE:  xor speed restored with bitshifting.  Collisions now entirely
 *          comparable with Bernstein
 */
__attribute__((hot)) static inline uint64_t data_path_hash( const char *str, int len )
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

	/*
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
	*/

	while( ctr > 4 )
	{
		sum ^= *p++;
		sum ^= *p++;
		sum ^= *p++;
		sum ^= *p++;
		ctr -=  4;

		// keep all bits involved
		sum <<= 4;
		sum += sum >> 32;
	}

	while( ctr-- > 0 )
		sum = ( sum << 1 ) ^ *p++;

	// and capture the rest
	str = (const char *) p;
	while( rem-- > 0 )
		sum += *str++ * data_path_hash_primes[rem];

	return sum;
}


uint64_t data_path_hash_wrap( const char *str, int len )
{
	return data_path_hash( str, len );
}



void data_keep_stats( ST_CFG *c )
{
	lock_stat_cfg( c );

	// and keep stats - gc reduces this
	++(c->dcurr);
	// but this is monotonic
	++(c->creates.count);

	unlock_stat_cfg( c );
}



__attribute__((hot)) static inline DHASH *data_find_path( DHASH *list, uint64_t hval, const char *path, int len )
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



DHASH *data_locate( const char *path, int len, int type )
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



__attribute__((hot)) static inline void data_get_tags( DHASH *d )
{
	char *p;

	if( !ctl->stats->tags_enabled )
		return;

	// if we have a tags separator, make some new space
	if( ( p = memchr( d->path, ctl->stats->tags_char, d->len ) ) )
	{
		d->blen = p - d->path;
		d->tlen = d->len - d->blen;

		// these are capped so are usable in strings
		d->base = str_copy( d->path, d->blen );
		d->tags = str_copy( p, d->tlen );
	}
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
			data_get_tags( d );
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

			data_get_tags( d );
			break;
	}
}


DHASH *data_find_dhash( const char *path, int len, ST_CFG *c )
{
	uint64_t hval, idx;

	hval = data_path_hash( (char *) path, len );
	idx  = hval % c->hsize;

	return data_find_path( c->data[idx], hval, (char *) path, len );
}


__attribute__((hot)) DHASH *data_create_dhash( const char *path, int len, ST_CFG *c, uint64_t hval, uint64_t idx )
{
	DHASH *n, *e;

	n = mem_new_dhash( path, len );

	if( !n )
	{
		fatal( "Could not allocate dhash for %s.", c->name );
		return NULL;
	}

	n->sum   = hval;
	n->type  = c->dtype;

	lock_table( idx );

	if( !( e = data_find_path( c->data[idx], hval, path, len ) ) )
	{
		n->next = c->data[idx];
		n->valid = 1;
		c->data[idx] = n;
	}

	unlock_table( idx );

	// someone else made it in-between
	// so free the new one and return that one
	// we don't want to do the free under lock,
	// as it has its own locking
	if( e )
	{
		//info( "Another thread created '%s' for us.", path );
		mem_free_dhash( &n );
		return e;
	}

	// finish setup
	data_get_dhash_extras( n );
	data_keep_stats( c );

	return n;
}



__attribute__((hot)) DHASH *data_get_dhash( const char *path, int len, ST_CFG *c )
{
	uint64_t hval, idx;
	DHASH *d;

	hval = data_path_hash( path, len );
	idx  = hval % c->hsize;

	if( ( d = data_find_path( c->data[idx], hval, path, len ) ) )
		return d;

	/* try again, under lock this time, assuming we will create it */
	return data_create_dhash( path, len, c, hval, idx );
}

