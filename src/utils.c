/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* utils.c - various routines for strings, config and maths                *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"

/*
 * This is a chunk of memory we hand back if calloc failed.
 *
 * The idea is that we hand back this highly distinctive chunk
 * of space.  Any pointer using it will fail.  Any attempt to
 * write to it should fail.
 *
 * So it should give us a core dump at once, right at the
 * problem.  Of course, threads will "help"...
 */

#ifdef USE_MEM_SIGNAL_ARRAY
static const uint8_t mem_signal_array[128] = {
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0
};
#endif


// zero'd memory
void *allocz( size_t size )
{
	void *p = calloc( 1, size );

#ifdef USE_MEM_SIGNAL_ARRAY
	if( !p )
		p = &mem_signal_array;
#endif

	return p;
}



void get_time( void )
{
	if( ctl )
		clock_gettime( CLOCK_REALTIME, &(ctl->curr_time) );
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
		return (char *) allocz( len );

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

	if( size )
	{
		b->space = (char *) allocz( size );
		b->sz    = size;
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
		l = strlen( str );

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
	pidfile_mkdir( ctl->pidfile );

	// and our file
	if( !( fh = fopen( ctl->pidfile, "w" ) ) )
	{	
		warn( "Unable to write to pidfile %s -- %s",
			ctl->pidfile, Err );
		return;
	}
	fprintf( fh, "%d\n", getpid( ) );
	fclose( fh );
}

void pidfile_remove( void )
{
	if( unlink( ctl->pidfile ) && errno != ENOENT )
		warn( "Unable to remove pidfile %s -- %s",
			ctl->pidfile, Err );
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
	ts_diff( ctl->curr_time, ctl->init_time, &diff );

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


int read_file( char *path, char **buf, int *len, size_t max, int perm, char *desc )
{
	struct stat sb;
	FILE *h;
	int l;

	if( !buf || !len )
	{
		err( "Need a buffer and length parameter in read_file." );
		return 1;
	}

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
	*len = l;

	if( l > (int) max )
	{
		err( "Size of %s %s is too big at %d bytes.", desc, path, l );
		return -4;
	}

	if( !( h = fopen( path, "r" ) ) )
	{
		err( "Cannot open %s %s: %s", desc, path, Err );
		return -5;
	}

	if( !*buf )
	{
		*buf = ( perm ) ? perm_str( l + 1 ) : (char *) allocz( l + 1 );
		debug( "Creating buffer of %d bytes for %s.", 1 + *len, desc );
	}

	if( !fread( *buf, l, 1, h ) || ferror( h ) )
	{
		err( "Could not read %s %s: %s", desc, path, Err );
		fclose( h );
		*len = 0;
		return -6;
	}

	fclose( h );
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

	if( !isdigit( *str ) )
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


