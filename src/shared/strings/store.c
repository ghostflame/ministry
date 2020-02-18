/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* strings/store.c - routines for string stores                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



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
		if( !st->hashtable[i] )
			continue;

		if( st->freefp )
		{
			// then call the free function where there is data
			for( e = st->hashtable[i]; e; e = e->next )
				if( e->ptr )
					(*(st->freefp))( e->ptr );
		}

		// otherwise that mess is the caller's problem

		mem_free_store_list( st->hashtable[i] );
		st->hashtable[i] = NULL;
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

	// and link it into _str
	stores_lock( );
	s->_str_next = _str->stores;
	_str->stores = s;
	stores_unlock( );

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
		 && len == e->len		// int <-> uint16_t comparison
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
		store_lock( store );
	else
		store_unlock( store );

	return 0;
}

SSTE *string_store_add_with_vals( SSTR *store, char *str, int len, int32_t *val, void *ptr )
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

	if( len > 0xffff )
	{
		err( "String is too long for string store - max %hu", 0xffff );
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
	en = mem_new_store( );

	// now check again under lock
	store_lock( store );

	for( e = store->hashtable[pos]; e; e = e->next )
		if( len == e->len && !memcmp( str, e->str, len ) )
			break;

	// find one?
	if( !e )
	{
		e  = en;
		en = NULL;
		e->str = ( store->freeable ) ? str_copy( str, len ) : str_dup( str, len );
		e->len = (uint16_t) len;
		e->hv  = hv;

		if( store->freeable )
			flagf_add( e, STRSTORE_FLAG_FREEABLE );

		if( val )
			e->val = *val;

		if( ptr )
			e->ptr = ptr;

		flagf_add( e, STRSTORE_FLAG_VALID );

		e->next = store->hashtable[pos];
		store->hashtable[pos] = e;

		++(store->entries);
	}

	// OK, renew the value
	if( !val && store->set_default )
		e->val = store->val_default;

	store_unlock( store );

	// did we actually create a new one and not use it?
	if( en )
		mem_free_store( &en );

	return e;
}


// divide the store's hash table into section
void __string_store_select_section( SSTR *store, int idx, int mod, int *start, int *end )
{
	int s, e, p;

	if( mod == 0 )
	{
		*start = 0;
		*end   = store->hsz;
		return;
	}

	p = store->hsz / mod;

	s =   idx       * p;
	e = ( idx + 1 ) * p;

	// make sure integer maths
	// doesn't mess with us
	// has uneven behaviour on very
	// small hash tables, but that's
	// unlikely to matter much
	if( idx == ( mod - 1 ) )
		e = store->hsz;

	*start = s;
	*end   = e;

	//info( "Selecting store section %d to %d (%d).", s, e, e - s );
}



// call a function on every member
int string_store_iterator( SSTR *store, void *arg, store_callback *fp, int idx, int mod )
{
	int i, ret = 0, stt, end;
	SSTE *e, *en;

	if( !store )
		return -1;

	// divvy up the workload between threads?
	__string_store_select_section( store, idx, mod, &stt, &end );

	store_lock( store );

	for( i = stt; i < end; ++i )
		for( e = store->hashtable[i]; e; e = en )
		{
			// just in case derp happens to e
			en = e->next;

			// count the non-zero e's
			if( (*fp)( e, arg ) != 0 )
				ret++;
		}

	store_unlock( store );

	return ret;
}



int string_store_iterator_nolock( SSTR *store, void *arg, store_callback *fp, int idx, int mod )
{
	int i, ret = 0, stt, end;
	SSTE *e, *en;

	if( !store )
	{
		warn( "NULL store passed to (nolock) iterator." );
		return -1;
	}

	__string_store_select_section( store, idx, mod, &stt, &end );

	// DOES NOT LOCK THE STORE!  We use a valid-flag test
	// whatever frees them must remove that flag before doing so

	for( i = stt; i < end; ++i )
		for( e = store->hashtable[i]; e; e = en )
		{
			en = e->next;

			// if that e is no longer valid, abort this list
			// we cannot continue, as next is no longer safe
			if( !flagf_has( e, STRSTORE_FLAG_VALID ) )
				break;

			// and call back on this one
			// checking it is valid is your problem
			if( (*fp)( e, arg ) != 0 )
				ret++;
		}

	return ret;
}


int64_t string_store_entries( SSTR *store )
{
	if( !store )
		return -1;

	return store->entries;
}



