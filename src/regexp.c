/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* regexp.c - implements a regex whitelist/blacklist                       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "ministry.h"


void regex_list_set_fallback( int fallback_match, RGXL *list )
{
	if( list )
		list->fb = ( fallback_match ) ? 0 : 1;
}


RGXL *regex_list_create( int fallback_match )
{
	RGXL *rl;

	rl = (RGXL *) allocz( sizeof( RGXL ) );
	regex_list_set_fallback( fallback_match, rl );
	return rl;
}



int regex_list_add( char *str, int negate, RGXL *rl )
{
	RGX *rg, *p;

	if( !rl || !str || !*str )
	{
		err( "No regex list %p, or invalid string %p", rl, str );
		return -1;
	}

	rg    = (RGX *) allocz( sizeof( RGX ) );
	rg->r = (regex_t *) allocz( sizeof( regex_t ) );

	if( regcomp( rg->r, str, REG_EXTENDED|REG_NOSUB ) )
	{
		err( "Could not compile regex string: %s", str );
		regfree( rg->r );
		free( rg );
		return -2;
	}

	rg->slen = strlen( str );
	rg->src  = str_dup( str, rg->slen );
	rg->ret  = ( negate ) ? 1 : 0;

	rl->count++;

	// first one?
	if( !rl->list )
	{
		rl->list = rg;
		return 0;
	}

	// append to list
	for( p = rl->list; p->next; p = p->next );

	p->next = rg;
	return 0;
}

int regex_list_test( char *str, RGXL *rl )
{
	RGX *r;

	// no list means MATCH
	if( !rl )
		return 0;

	for( r = rl->list; r; r = r->next )
	{
		r->tests++;
		//debug( "Checking '%s' against regex '%s'", str, r->src );
		if( regexec( r->r, str, 0, NULL, 0 ) == 0 )
		{
			r->matched++;
			return r->ret;
		}
	}

	return rl->fb;
}


