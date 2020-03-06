/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* strings/strings.c - routines for string handling                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"




char *str_perm( const char *src, int len )
{
	char *p;

	if( !len )
		len = strlen( src );

	p = mem_perm( (uint32_t) ( len + 1 ) );
	memcpy( p, src, len );
	p[len] = '\0';

	return p;
}

char *str_copy( const char *src, int len )
{
	char *p;

	if( !len )
		len = strlen( src );

	p = (char *) allocz( len + 1 );
	memcpy( p, src, len );

	return p;
}



// a capped version of strlen
int str_nlen( const char *src, int max )
{
	char *p;

	if( ( p = memchr( src, '\0', max ) ) )
		return p - src;

	return max;
}



int str_search( const char *str, const char **list, int len )
{
	while( len-- > 0 )
		if( !strcasecmp( str, list[len] ) )
			break;

	return len;
}



//
// String substitute
// Creates a new string, and needs an arg list
//
// Implements %\d% substitution
//
static regex_t *sub_rgx = NULL;
#define SUB_REGEX	"^(.*?)%([1-9]+)%(.*)$"

int str_sub( char **ptr, int *len, int argc, char **argv, int *argl )
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
			warn( "str_sub: arg number is too long (%d bytes).", l );
			return c;
		}

		memcpy( numbuf, a + mtc[2].rm_so, l );
		numbuf[l] = '\0';

		if( ( i = strtol( numbuf, NULL, 10 ) ) > argc )
		{
			warn( "str_sub: config substitution referenced non-existent arg %d, aborting.", i );
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
		++c;
	}

#ifdef DEBUG
	if( c > 0 )
		debug( "str_sub: (%d) (%s) -> (%s)", c, o, *ptr );
#endif

	return c;
}


// remove trailing newlines\carriage returns,
// report the amount removed
int chomp( char *s, int len )
{
	char *p;
	int l;

	if( !s || !*s )
		return 0;

	if( len )
		l = len;
	else
		l = strlen( s );

	p = s + ( l - 1 );

	while( l > 0 && ( *p == '\n' || *p == '\r' ) )
	{
		*p-- = '\0';
		l--;
	}

	return len - l;
}

// remove leading and trailing whitespace
__attribute__((hot)) int trim( char **str, int *len )
{
	register char *p, *q;
	register int l, o;

	if( !str || !*str || !**str || !len || !*len )
		return 0;

	p = *str;
	l = *len;
	o = *len;
	q = p + ( l - 1 );

	while( l > 0 && isspace( *p ) )
	{
		p++;
		l--;
	}
	while( l > 0 && isspace( *q ) )
	{
		*q-- = '\0';
		l--;
	}

	*str = p;
	*len = l;

	return o - l;
}


// compare two strings as potential for one prefix of the other
int strprefix( const char *pr, const char *of )
{
	while( *pr && *of && *pr == *of )
	{
		++pr;
		++of;
	}

	// did we run out of prefix?
	return ( *pr ) ? 1 : 0;
}


