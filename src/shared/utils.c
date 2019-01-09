/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* utils.c - various routines for strings, config and maths                *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"



void get_time( void )
{
	if( _proc )
	{
		clock_gettime( CLOCK_REALTIME, &(_proc->curr_time) );
		_proc->curr_tval = tsll( _proc->curr_time );
	}
}

int64_t get_time64( void )
{
	struct timespec ts;

	clock_gettime( CLOCK_REALTIME, &ts );
	return tsll( ts );
}



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
		sz       = mem_alloc_size( 24 + size );
		b->space = (char *) allocz( sz );
		b->sz    = (uint32_t) sz;
	}

	b->buf = b->space;
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



int strwords( WORDS *w, char *src, int len, char sep )
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




#define	VV_NO_CHECKS	0x07
int var_val( char *line, int len, AVP *av, int flags )
{
	char *a, *v, *p, *q, *r, *s;

	if( !line || !av )
		return 1;

	if( ( flags & VV_NO_CHECKS ) == VV_NO_CHECKS )
		flags |= VV_NO_VALS;

	/* blat it */
	memset( av, 0, sizeof( AVP ) );

	p  = line;     // start of line
	r  = line;     // needs init
	s  = p + len;  // end of line
	*s = '\0';     // stamp on the newline

	while( len && isspace( *p ) )
	{
		p++;
		len--;
	}
	if( !( flags & VV_VAL_WHITESPACE )
	 || ( flags & VV_NO_VALS ) )
	{
		while( s > p && isspace( *(s-1) ) )
		{
			*--s = '\0';
			len--;
		}
	}

	/* blank line... */
	if( len < 1 )
	{
		av->status = VV_LINE_BLANK;
		return 0;
	}

	/* comments are successfully read */
	if( *p == '#' )
	{
		av->status = VV_LINE_COMMENT;
		return 0;
	}

	// if we are ignoring values, what we have is an attribute
	if( ( flags & VV_NO_VALS ) )
	{
		a = p;
		av->alen = len;

		// if we're automatically setting vals, set 1
		if( flags & VV_AUTO_VAL )
		{
			v  = "1";
			av->vlen = 1;
		}
		else
			v = "";
	}
	else
	{
		/* search order is =, tab, space */
		q = NULL;
		if( !( flags & VV_NO_EQUAL ) )
			q = memchr( p, '=', len );
		if( !q && !( flags & VV_NO_TAB ) )
			q = memchr( p, '\t', len );
		if( !q && !( flags & VV_NO_SPACE ) )
			q = memchr( p, ' ', len );

		if( !q )
		{
			// are we accepting flag values - no value
			if( flags & VV_AUTO_VAL )
			{
				// then we have an attribute and an auto value
				a        = p;
				av->alen = s - p;
				v        = "1";
				av->vlen = 1;
				goto vv_finish;
			}

			av->status = VV_LINE_BROKEN;
			return 1;
		}

		// blat the separator
		*q = '\0';
		r  = q + 1; 

		if( !( flags & VV_VAL_WHITESPACE ) )
		{
			// start off from here with the value
			while( r < s && isspace( *r ) )
				r++;
		}

		while( q > p && isspace( *(q-1) ) )
			*--q = '\0';

		// OK, let's record those
		a        = p;
		av->alen = q - p;
		v        = r;
		av->vlen = s - r;
	}

	/* got an attribute?  No value might be legal but this ain't */
	if( !av->alen )
	{
		av->status = VV_LINE_NOATT;
		return 1;
	}

vv_finish:

	if( av->alen >= AVP_MAX_ATT )
	{
		av->status = VV_LINE_AOVERLEN;
		return 1;
	}
	if( av->vlen >= AVP_MAX_VAL )
	{
		av->status = VV_LINE_VOVERLEN;
		return 1;
	}

	// were we squashing underscores in attrs?
	if( flags && VV_REMOVE_UDRSCR )
	{
		// we are deprecating key names with _ in them, to enable
		// supporting environment names where we will need to
		// replace _ with . to support the hierarchical keys
		// so warn about any key with _ in it.

		if( memchr( a, '_', av->alen ) )
			warn( "Key %s contains an underscore _, this is deprecated in favour of camelCase.", a );

		// copy without underscores
		for( p = a, q = av->att; *p; p++ )
			if( *p != '_' )
				*q++ = *p;

		// cap it
		*q = '\0';

		// and recalculate the length
		av->alen = q - av->att;
	}
	else
	{
		memcpy( av->att, a, av->alen );
		av->att[av->alen] = '\0';
	}

	memcpy( av->val, v, av->vlen );
	av->val[av->vlen] = '\0';

	// are were lower-casing the attributes?
	if( flags && VV_LOWER_ATT )
		for( p = av->att, q = p + av->alen; p < q; p++ )
			*p = tolower( *p );

	/* looks good */
	av->status = VV_LINE_ATTVAL;
	return 0;
}




void pidfile_mkdir( char *path )
{
	char dbuf[2048], *ls;
	struct stat sb;
	int l;

	// we have a sensible looking path?
	if( !( ls = strrchr( path, '/' ) )
	 || ls == path )
		return;

	// path too long for us?
	if( ( l = ls - path ) > 2047 )
		return;

	memcpy( dbuf, path, l );
	dbuf[l] = '\0';

	// prune trailing /
	while( l > 1 && dbuf[l-1] == '/' )
		dbuf[--l] = '\0';

	// recurse up towards /
	pidfile_mkdir( dbuf );

	if( stat( dbuf, &sb ) )
	{
		if( errno == ENOENT )
		{
			if( mkdir( dbuf, 0755 ) )
				warn( "Could not create pidfile path parent dir %s -- %s",
					dbuf, Err );
			else
				info( "Created pidfile path parent dir %s", dbuf );
		}
		else
			warn( "Cannot stat pidfile parent dir %s -- %s", dbuf, Err );
	}
	else if( !S_ISDIR( sb.st_mode ) )
	{
		warn( "Pidfile parent %s is not a directory.", dbuf );
	}
}



void pidfile_write( void )
{
	FILE *fh;

	// make our piddir
	pidfile_mkdir( _proc->pidfile );

	// and our file
	if( !( fh = fopen( _proc->pidfile, "w" ) ) )
	{	
		warn( "Unable to write to pidfile %s -- %s",
			_proc->pidfile, Err );
		return;
	}
	fprintf( fh, "%d\n", getpid( ) );
	fclose( fh );
}

void pidfile_remove( void )
{
	if( unlink( _proc->pidfile ) && errno != ENOENT )
		warn( "Unable to remove pidfile %s -- %s",
			_proc->pidfile, Err );
}







double ts_diff( struct timespec to, struct timespec from, double *store )
{
	double diff;

	diff  = (double) ( to.tv_nsec - from.tv_nsec );
	diff /= 1000000000.0;
	diff += (double) ( to.tv_sec  - from.tv_sec  );

	if( store )
		*store = diff;

	return diff;
}


double get_uptime( void )
{
	double diff;

	get_time( );
	ts_diff( _proc->curr_time, _proc->init_time, &diff );

	return diff;
}



int setlimit( int res, int64_t val )
{
	char *which = "unknown";
	struct rlimit rl;

	switch( res )
	{
		case RLIMIT_NOFILE:
			which = "files";
			break;
		case RLIMIT_NPROC:
			which = "procs";
			break;
	}

	getrlimit( res, &rl );

	// -1 means GIVE ME EVERYTHING!
	if( val == -1 )
		rl.rlim_cur = rl.rlim_max;
	else
	{
		if( (int64_t) rl.rlim_max < val )
		{
			err( "Limit %s max is %ld.", which, rl.rlim_max );
			return -1;
		}
		rl.rlim_cur = val;
	}

	if( setrlimit( res, &rl ) )
	{
		err( "Cannot set %s limit to %ld: %s",
			which, val, Err );
		return -1;
	}

	debug( "Set %s limit to %ld.", which, rl.rlim_cur );
	return 0;
}


uint64_t lockless_fetch( LLCT *l )
{
	uint64_t diff, curr;

	// we can only read from it *once*
	// then we do maths
	curr = l->count;
	diff = curr - l->prev;

	// and set the previous
	l->prev = curr;

	return diff;
}


int read_file( char *path, char **buf, int *len, int perm, char *desc )
{
	int l, max, r, fd;
	struct stat sb;

	if( !buf || !len )
	{
		err( "Need a buffer and length parameter in read_file." );
		return 1;
	}

	max = *len;

	if( !desc )
		desc = "file";

	if( !path )
	{
		err( "No %s path.", desc );
		return -1;
	}

	if( stat( path, &sb ) )
	{
		err( "Cannot stat %s %s: %s", desc, path, Err );
		return -2;
	}

	if( !S_ISREG( sb.st_mode ) )
	{
		err( "Mode of %s %s is not a regular file.", desc, path );
		return -3;
	}

	l = (int) sb.st_size;

	// files in /proc stat as 0 bytes, helpfully
	if( l > 0 )
		*len = l;

	if( l > max )
	{
		err( "Size of %s %s is too big at %d bytes.", desc, path, l );
		return -4;
	}

	if( ( fd = open( path, O_RDONLY ) ) < 0 )
	{
		err( "Cannot open %s %s: %s", desc, path, Err );
		return -5;
	}

	if( !*buf )
	{
		// we need a buffer if you want to read from /proc
		*buf = ( perm ) ? perm_str( l + 1 ) : (char *) allocz( l + 1 );
		debug( "Creating buffer of %d bytes for %s.", 1 + *len, desc );
	}
	else
		memset( *buf, 0, max );

	// with /proc we just read what we can get
	if( l == 0 && max > 0 )
		r = max;
	else
		r = l;

	if( ( *len = read( fd, *buf, r ) ) < 0 )
	{
		err( "Could not read %s %s: %s", desc, path, Err );
		close( fd );
		*len = 0;
		return -6;
	}

	close( fd );
	return 0;
}


int parse_number( char *str, int64_t *iv, double *dv )
{
	if( iv )
		*iv = 0;
	if( dv )
		*dv = 0;

	if( !strncmp( str, "0x", 2 ) )
	{
		if( iv )
			*iv = strtoll( str, NULL, 16 );
		return NUM_HEX;
	}

	while( *str == '+' )
		str++;

	if( *str != '-' && !isdigit( *str ) )
		return NUM_INVALID;

	if( strchr( str, '.' ) )
	{
		if( dv )
			*dv = strtod( str, NULL );
		return NUM_FLOAT;
	}

	if( strlen( str ) > 1 && *str == '0' )
	{
		if( iv )
			*iv = strtoll( str, NULL, 8 );
		return NUM_OCTAL;
	}

	if( iv )
		*iv = strtoll( str, NULL, 10 );
	if( dv )
		*dv = strtod( str, NULL );

	return NUM_NORMAL;
}



uint64_t hash_size( char *str )
{
	int64_t v = 0;

	if( !str || !strlen( str ) )
	{
		warn( "Invalid hashtable size string." );
		return 0;
	}

	if( !strcasecmp( str, "nano" ) )
		return MEM_HSZ_NANO;

	if( !strcasecmp( str, "micro" ) )
		return MEM_HSZ_MICRO;

	if( !strcasecmp( str, "tiny" ) )
		return MEM_HSZ_TINY;

	if( !strcasecmp( str, "small" ) )
		return MEM_HSZ_SMALL;

	if( !strcasecmp( str, "medium" ) )
		return MEM_HSZ_MEDIUM;

	if( !strcasecmp( str, "large" ) )
		return MEM_HSZ_LARGE;

	if( !strcasecmp( str, "xlarge" ) )
		return MEM_HSZ_XLARGE;

	if( !strcasecmp( str, "x2large" ) )
		return MEM_HSZ_X2LARGE;

	if( parse_number( str, &v, NULL ) == NUM_INVALID )
	{
		warn( "Unrecognised hash table size '%s'", str );
		return -1;
	}

	return (uint64_t) v;
}


static const uint64_t util_str_hash_primes[8] =
{
	2909, 3001, 3083, 3187, 3259, 3343, 3517, 3581
};


/*
 * This is loosely based on DJ Bernsteins per-character hash, but with
 * significant speedup from the 32-bit in pointer cast.
 *
 * It replaces an xor based hash that showed too many collisions.
 */
__attribute__((hot)) static inline uint64_t util_str_hash( char *str, int len )
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
		sum += *str++ * util_str_hash_primes[rem];

	return sum;
}



int is_url( char *str )
{
	int l = strlen( str );	

	if( l < 8 )
		return STR_URL_NO;

	if( !strncasecmp( str, "http://", 7 ) )
		return STR_URL_YES;

	if( !strncasecmp( str, "https://", 8 ) )
		return STR_URL_SSL;

	return STR_URL_NO;
}



void string_store_cleanup( SSTR *list )
{
	while( list )
	{
		pthread_mutex_destroy( &(list->mtx) );
		list = list->next;
	}
}


SSTR *string_store_create( int64_t sz, char *size )
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

	hv = util_str_hash( str, len );

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
		e->val = store->val_default;
		return e;
	}

	hv = util_str_hash( str, len ) % store->hsz;

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
	e->val = store->val_default;
	pthread_mutex_unlock( &(store->mtx) );

	// did we actually create a new one?
	if( en )
		free( en );

	return e;
}


