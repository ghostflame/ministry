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
	char *p, *q, *r, *s;

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
		av->att = p;
		av->alen = len;

		// if we're automatically setting vals, set 1
		if( flags & VV_AUTO_VAL )
		{
			av->val  = "1";
			av->vlen = 1;
		}
		else
			av->val = "";
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
				av->att  = p;
				av->alen = s - p;
				av->val  = "1";
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
		av->att = p;
		av->alen = q - p;
		av->val = r;
		av->vlen = s - r;
	}

	/* got an attribute?  No value might be legal but this ain't */
	if( !av->alen )
	{
		av->status = VV_LINE_NOATT;
		return 1;
	}

vv_finish:

	// are were lower-casing the attributes?
	if( flags && VV_LOWER_ATT )
		for( p = av->att, q = p + av->alen; p < q; p++ )
			*p = tolower( *p );

	/* looks good */
	av->status = VV_LINE_ATTVAL;
	return 0;
}


void pidfile_write( void )
{
	FILE *fh;

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


// an implementation of Kaham Summation
// https://en.wikipedia.org/wiki/Kahan_summation_algorithm
// useful to avoid floating point errors
static inline void kahan_sum( double val, double *sum, double *low )
{
	double y, t;

	y = val - *low;		// low starts off small
	t = *sum + y;		// sum is big, y small, lo-order y is lost

	*low = ( t - *sum ) - y;// (t-sum) is hi-order y, -y recovers lo-order
	*sum = t;		// low is algebraically always 0
}

void kahan_summation( double *list, int len, double *sum )
{
	double low = 0;
	int i;

	for( *sum = 0, i = 0; i < len; i++ )
		kahan_sum( list[i], sum, &low );

	*sum += low;
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



