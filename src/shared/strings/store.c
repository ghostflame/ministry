/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* strings/store.c - routines for string stores                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"



static const uint64_t str_hash_primes[8] =
{
	2909, 3001, 3083, 3187, 3259, 3343, 3517, 3581
};



/*
 * This is loosely based on DJ Bernsteins per-character hash, but with
 * significant speedup from the 32-bit in pointer cast.
 *
 * It replaces an xor based hash that showed too many collisions.
 */
__attribute__((hot)) static inline uint64_t str_hash( char *str, int len )
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
		sum += *str++ * str_hash_primes[rem];

	return sum;
}



void string_store_cleanup( SSTR *list )
{
	while( list )
	{
		pthread_mutex_destroy( &(list->mtx) );
		list = list->next;
	}
}


// empty a string store, but keep the hash table and size
int string_store_empty( SSTR *st )
{
	int64_t i;
	SSTE *e;

	if( !st->freeable )
	{
		err( "Cannot empty a string store that was not created freeable." );
		return -1;
	}

	for( i = 0; i < st->hsz; ++i )
	{
		while( st->hashtable[i] )
		{
			e = st->hashtable[i];
			st->hashtable[i] = e->next;

			// do we have a free function?
			if( e->ptr && st->freefp )
				(*(st->freefp))( e->ptr );

			// otherwise that mess is the callers problem
			free( e->str );
			free( e );
		}
	}

	return 0;
}


int string_store_free_cb( SSTR *st, mem_free_cb *fp )
{
	if( st )
	{
		st->freefp = fp;
		return 0;
	}

	return -1;
}


SSTR *string_store_create( int64_t sz, char *size, int *default_value, int freeable )
{
	SSTR *s = (SSTR *) allocz( sizeof( SSTR ) );

	if( sz )
		s->hsz = sz;
	else if( size )
		s->hsz = hash_size( size );

	if( !s->hsz )
		s->hsz = hash_size( "medium" );

	s->hashtable = (SSTE **) allocz( s->hsz * sizeof( SSTE * ) );
	s->freeable  = freeable;

	pthread_mutex_init( &(s->mtx), NULL );

	if( default_value )
	{
		s->val_default = *default_value;
		s->set_default = 1;
	}

	// and link it into _proc
	config_lock( );

	s->_proc_next = _proc->stores;
	_proc->stores = s;

	config_unlock( );

	return s;
}



SSTE *string_store_look( SSTR *store, char *str, int len, int val_set )
{
	uint64_t hv;
	SSTE *e;

	if( !store || !store->hashtable )
	{
		err( "NULL or invalid string store (%p) passed to string_store_look.", store );
		return NULL;
	}

	if( !str || !*str )
	{
		err( "NULL or invalid string (%p) passed to string_store_look.", str );
		return NULL;
	}

	if( !len )
		len = strlen( str );

	hv = str_hash( str, len );

	// we can do this without lock because adding a new string
	// is atomic (overwriting store->hash[hv]), and setting
	// e->val to zero would also be atomic
	for( e = store->hashtable[hv % store->hsz]; e; e = e->next )
		if( hv == e->hv
		 && len == e->len
		 && !memcmp( str, e->str, len )
		 && ( val_set == 0 || e->val ) )
			return e;
 
	return NULL;
}

int string_store_locking( SSTR *store, int lk )
{
	if( !store )
		return -1;

	if( lk )
		pthread_mutex_lock( &(store->mtx) );
	else
		pthread_mutex_unlock( &(store->mtx) );

	return 0;
}



SSTE *string_store_add( SSTR *store, char *str, int len )
{
	uint64_t hv, pos;
	SSTE *e, *en;

	if( !store || !store->hashtable )
	{
		err( "NULL or invalid string store (%p) passed to string_store_add.", store );
		return NULL;
	}

	if( !str || !*str )
	{
		err( "NULL or invalid string (%p) passed to string_store_add.", str );
		return NULL;
	}

	// do we already have that?  Silently return the existing one
	// we'll give it the standard value again
	if( ( e = string_store_look( store, str, len, 0 ) ) )
	{
		if( store->set_default )
			e->val = store->val_default;
		return e;
	}

	hv = str_hash( str, len );
	pos = hv % store->hsz;

	// don't allocz under lock, as we might brk()
	en = (SSTE *) allocz( sizeof( SSTE ) );

	// now check again under lock
	pthread_mutex_lock( &(store->mtx) );

	for( e = store->hashtable[pos]; e; e = e->next )
		if( len == e->len && !memcmp( str, e->str, len ) )
			break;

	// find one?
	if( !e )
	{
		e  = en;
		en = NULL;
		e->str = ( store->freeable ) ? str_copy( str, len ) : str_dup( str, len );
		e->len = len;
		e->hv  = hv;

		e->next = store->hashtable[pos];
		store->hashtable[pos] = e;

		++(store->entries);
	}

	// OK, renew the value
	if( store->set_default )
		e->val = store->val_default;

	pthread_mutex_unlock( &(store->mtx) );

	// did we actually create a new one and not use it?
	if( en )
		free( en );

	return e;
}

int64_t string_store_entries( SSTR *store )
{
	if( !store )
		return -1;

	return store->entries;
}



