/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* regexp.c - implements a regex match/unmatch list                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "shared.h"


void regex_list_set_fallback( int fallback_match, RGXL *list )
{
	if( list )
		list->fb = ( fallback_match ) ? REGEX_MATCH : REGEX_FAIL;
}


RGXL *regex_list_create( int fallback_match )
{
	RGXL *rl;

	rl = (RGXL *) allocz( sizeof( RGXL ) );
	regex_list_set_fallback( fallback_match, rl );
	return rl;
}

void regex_list_destroy( RGXL *rl )
{
	RGX *r;

	if( !rl )
		return;

	while( rl->list )
	{
		r = rl->list;
		rl->list = r->next;

		free( r->src );
		regfree( &(r->r) );
		free( r );
	}

	free( rl );
}


int regex_list_add( char *str, int negate, RGXL *rl )
{
	RGX *rg;

	if( !rl || !str || !*str )
	{
		err( "No regex list %p, or invalid string %p", rl, str );
		return -1;
	}

	rg = (RGX *) allocz( sizeof( RGX ) );

	// reverse the sense with a leading !
	if( *str == '!' )
	{
		negate = !negate;
		++str;
	}

	rg->slen = strlen( str );
	rg->src  = str_copy( str, rg->slen );
	rg->ret  = ( negate ) ? REGEX_FAIL : REGEX_MATCH;

	if( regcomp( &(rg->r), str, REG_EXTENDED|REG_NOSUB ) )
	{
		err( "Could not compile regex string: %s", str );
		regfree( &(rg->r) );
		free( rg->src );
		free( rg );
		return -2;
	}

	++(rl->count);

	// first one?
	if( !rl->list )
	{
		rl->list = rg;
		rl->last = rg;
	}
	else
	{
		rl->last->next = rg;
		rl->last = rg;
	}

	return 0;
}

int regex_list_test( char *str, RGXL *rl )
{
	RGX *r;

	// no list means MATCH
	if( !rl )
		return REGEX_MATCH;

	for( r = rl->list; r; r = r->next )
	{
		++(r->tests);
		//debug( "Checking '%s' against regex '%s'", str, r->src );
		if( regexec( &(r->r), str, 0, NULL, 0 ) == 0 )
		{
			++(r->matched);
			return r->ret;
		}
	}

	return rl->fb;
}


