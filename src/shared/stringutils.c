/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* strings.c - routines for string handling                                *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"



char *perm_space_ptr  = NULL;
char *perm_space_curr = NULL;
int   perm_space_left = 0;


char *perm_str( int len )
{
	char *p;

	// ensure we are 4-byte aligned
	if( len & 0x3 )
		len += 4 - ( len % 4 );

	// just malloc big blocks
	if( len >= ( PERM_SPACE_BLOCK >> 8 ) )
	{
		return (char *) allocz( len );
	}

	if( len > perm_space_left )
	{
		perm_space_ptr  = (char *) allocz( PERM_SPACE_BLOCK );
		perm_space_curr = perm_space_ptr;
		perm_space_left = PERM_SPACE_BLOCK;
	}

	p = perm_space_curr;
	perm_space_curr += len;
	perm_space_left -= len;

	return p;
}

char *str_dup( char *src, int len )
{
	char *p;

	if( !len )
		len = strlen( src );

	p = perm_str( len + 1 );
	memcpy( p, src, len );
	p[len] = '\0';

	return p;
}

char *str_copy( char *src, int len )
{
	char *p;

	if( !len )
		len = strlen( src );

	p = (char *) allocz( len + 1 );
	memcpy( p, src, len );

	return p;
}

// permanent string buffer
BUF *strbuf( uint32_t size )
{
	BUF *b = (BUF *) allocz( sizeof( BUF ) );
	size_t sz;

	if( size )
	{
		// make a little room
		if( size < 128 )
			size += 24;

		sz       = mem_alloc_size( size );
		b->space = (char *) allocz( sz );
		b->sz    = (uint32_t) sz;
	}

	b->buf = b->space;
	return b;
}

BUF *strbuf_resize( BUF *b, uint32_t size )
{
	size_t sz;

	if( !size )
		return NULL;

	if( !b )
		return strbuf( size );

	if( size < 128 )
		size += 24;

	sz = mem_alloc_size( size );

	if( sz > b->sz )
	{
		free( b->space );
		b->space  = (char *) allocz( sz );
		b->sz     = sz;
		b->buf    = b->space;
	}

	b->len    = 0;
	b->buf[0] = '\0';

	return b;
}

int strbuf_copy( BUF *b, char *str, int len )
{
	if( !len )
		len = strlen( str );

	if( (uint32_t) len >= b->sz )
		return -1;

	memcpy( b->buf, str, len );
	b->len = len;
	b->buf[b->len] = '\0';

	return len;
}

int strbuf_add( BUF *b, char *str, int len )
{
	if( !len )
		len = strlen( str );

	if( ( b->len + len ) >= b->sz )
		return -1;

	memcpy( b->buf + b->len, str, len );
	b->len += len;
	b->buf[b->len] = '\0';

	return len;
}

BUF *strbuf_create( char *str, int len )
{
	int k, l;
	BUF *b;

	if( !len )
		len = strlen( str );
	if( !len )
		len = 1;

	for( k = 0, l = len; l > 0; k++ )
		l = l >> 1;

	b = strbuf( 1 << k );
	strbuf_add( b, str, len );

	return b;
}


// a capped version of strlen
int str_nlen( char *src, int max )
{
	char *p;

	if( ( p = memchr( src, '\0', max ) ) )
		return p - src;

	return max;
}


//
// String substitute
// Creates a new string, and needs an arg list
//
// Implements %\d% substitution
//
static regex_t *sub_rgx = NULL;
#define SUB_REGEX	"^(.*?)%([1-9]+)%(.*)$"

int strsub( char **ptr, int *len, int argc, char **argv, int *argl )
{
	char *a, *b, numbuf[8];
	regmatch_t mtc[4];
	int i, l, c = 0;
#ifdef DEBUG
	char *o;
#endif

	if( !sub_rgx )
	{
		sub_rgx = (regex_t *) allocz( sizeof( regex_t ) );
		if( regcomp( sub_rgx, SUB_REGEX, REG_EXTENDED ) != 0 )
		{
			fatal( "Failed to compile string substitution regex." );
			return -1;
		}
	}

	// start with a copy
	a = str_copy( *ptr, *len );
#ifdef DEBUG
	o = *ptr;
#endif

	while( regexec( sub_rgx, a, 4, mtc, 0 ) == 0 )
	{
		l = mtc[2].rm_eo - mtc[2].rm_so;

		if( l > 7 )
		{
			warn( "strsub: arg number is too long (%d bytes).", l );
			return c;
		}

		memcpy( numbuf, a + mtc[2].rm_so, l );
		numbuf[l] = '\0';

		if( ( i = atoi( numbuf ) ) > argc )
		{
			warn( "strsub: config substitution referenced non-existent arg %d, aborting.", i );
			return c;
		}

		// args are 0-indexed
		i--;

		// stomp on the first %, it makes copying easier
		a[mtc[1].rm_eo] = '\0';

		// get the new length
		l = mtc[1].rm_eo + argl[i] + ( mtc[3].rm_eo - mtc[3].rm_so );
		b = (char *) allocz( l + 1 );
		snprintf( b, l + 1, "%s%s%s", a, argv[i], a + mtc[3].rm_so );

		free( a );
		a = b;

		*ptr = a;
		*len = l;
		c++;
	}

#ifdef DEBUG
	if( c > 0 )
		debug( "strsub: (%d) (%s) -> (%s)", c, o, *ptr );
#endif

	return c;
}




int strqwords( WORDS *w, char *src, int len, char sep )
{
	register char *p = src;
	register char *q = NULL;
	register int   l;
	int i = 0, qc = 0, qchar;

	if( !w || !p || !sep )
		return -1;

	if( !len && !( len = strlen( p ) ) )
		return 0;

	l = len;

	memset( w, 0, sizeof( WORDS ) );

	w->in_len = l;
	qchar     = '"';

	// step over leading separators
	while( *p == sep )
	{
		p++;
		l--;
	}

	// anything left?
	if( !*p )
		return 0;

	while( l > 0 )
	{
		// decision: "" wraps ''
		if( *p == '"' )
		{
			qc = 1;
			qchar = '"';
		}

		if( !qc && *p == '\'' )
		{
			qc = 1;
			qchar = '\'';
		}

		if( qc )
		{
			p++;
			l--;

			w->wd[i] = p;

			if( ( q = memchr( p, qchar, l ) ) )
			{
				// capture inside the quotes
				w->len[i++] = q - p;
				*q++ = '\0';
				l -= q - p;
				p = q;
				qc = 0;
			}
			else
			{
				// invalid string - uneven quotes
				// assume to-end-of-line for the
				// quoted string
				w->len[i++] = l;
				break;
			}

			// step over any separators following the quotes
			while( *p == sep )
			{
				p++;
				l--;
			}
		}
		else
		{
			w->wd[i] = p;

			if( ( q = memchr( p, sep, l ) ) )
			{
				w->len[i++] = q - p;
				*q++ = '\0';
				l -= q - p;
				p = q;
			}
			else
			{
				w->len[i++] = l;
				break;
			}
		}

		// note any remaining we didn't capture
		// due to size constraints
		if( i == STRWORDS_MAX )
		{
			w->end = p;
			w->end_len = l;
			break;
		}
	}

	return ( w->wc = i );
}



int strwords_multi( WORDS *w, char *src, int len, char sep, int8_t multi )
{
	register char *p = src;
	register char *q = NULL;
	register int   l;
	int i = 0;

	if( !w || !p || !sep )
		return -1;

	if( !len && !( len = strlen( p ) ) )
		return 0;

	l = len;

	memset( w, 0, sizeof( WORDS ) );

	w->in_len = l;

	// step over leading separators
	while( *p == sep )
	{
		p++;
		l--;
	}

	// anything left?
	if( !*p )
		return 0;

	// and break it up
	while( l )
	{
		w->wd[i] = p;

		if( ( q = memchr( p, sep, l ) ) )
		{
			w->len[i++] = q - p;
			if( multi )
				while( *q == sep )
					*q++ = '\0';
			else
				*q++ = '\0';

			l -= q - p;
			p = q;
		}
		else
		{
			w->len[i++] = l;
			break;
		}

		// note any remaining we didn't capture
		// due to size constraints
		if( i == STRWORDS_MAX )
		{
			w->end = p;
			w->end_len = l;
			break;
		}
	}

	// done
	return ( w->wc = i );
}


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


SSTR *string_store_create( int64_t sz, char *size, int *default_value )
{
	SSTR *s = (SSTR *) allocz( sizeof( SSTR ) );

	if( sz )
		s->hsz = sz;
	else if( size )
		s->hsz = hash_size( size );

	if( s->hsz )
		s->hsz = hash_size( "medium" );

	s->hashtable = (SSTE **) allocz( s->hsz * sizeof( SSTE * ) );

	pthread_mutex_init( &(s->mtx), NULL );

	if( default_value )
	{
		s->val_default = *default_value;
		s->set_default = 1;
	}

	// and link it into _proc
	s->_proc_next = _proc->stores;
	_proc->stores = s;

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

	hv = str_hash( str, len ) % store->hsz;

	// we can do this without lock because adding a new string
	// is atomic (overwriting store->hash[hv]), and setting
	// e->val to zero would also be atomic
	for( e = store->hashtable[hv]; e; e = e->next )
		if( len == e->len
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
	uint64_t hv;
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

	hv = str_hash( str, len ) % store->hsz;

	// don't allocz under lock, as we might brk()
	en = (SSTE *) allocz( sizeof( SSTE ) );

	// now check again under lock
	pthread_mutex_lock( &(store->mtx) );

	for( e = store->hashtable[hv]; e; e = e->next )
		if( len == e->len && !memcmp( str, e->str, len ) )
			break;

	// find one?
	if( !e )
	{
		e  = en;
		en = NULL;
		e->str = str_dup( str, len );
		e->len = len;

		e->next = store->hashtable[hv];
		store->hashtable[hv] = e;

		store->entries++;
	}

	// OK, renew the value
	if( store->set_default )
		e->val = store->val_default;

	pthread_mutex_unlock( &(store->mtx) );

	// did we actually create a new one?
	if( en )
		free( en );

	return e;
}

