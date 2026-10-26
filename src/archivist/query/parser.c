/**************************************************************************
* Copyright 2015 John Denholm                                             *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
*                                                                         *
*                                                                         *
* query/parser.c - query parsing functions                                *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


QRFN *query_parse_function( QRY *q, char *name, int len )
{
	QRFN *fp;

	for( fp = query_function_defns; fp->name != NULL; ++fp )
	{
		if( fp->nlen == len && !strncasecmp( name, fp->name, len ) )
		{
			debug( "Identified function '%s' from '%s'\n", fp->name, name );
			return fp;
		}
	}

	return NULL;
}


// we have section of query, so we have a start and a length
int query_detect_arg_type( QLYR *ql, QARG *qa )
{
	long double d;
	char *p, *q;
	int64_t i;

	if( ( p = memchr( qa->val.s, '.', qa->len ) ) )
	{

	}
	else
	{
		i = strtroll( qa->val.s, &q, 10 );

	}
}


/*
Parsing scheme:
 a/ blah.foo.bar
 b/ blah.foo.*
 c/ blah.foo.bar,blah.foo.eek
 d/ some_function(blah.foo.bar)
 e/ some_function(other_function(blah.foo.bar))
 f/ multi_function(blah.foo.bar, blah.foo.eek)
 g/ weird_function(blah.foo.bar,other_function(blah.foo.bar))
 h/ complex_function(blah.foo.bar,other_function(blah.foo.eek),blah.foo.woo)

 SO
 Seek ( && seek ,
	start to ( is function name
		check function name
	seek last )
	enclosed string is the arguments
*/

int query_parse_args( QRY *q, QLYR *ql, char *src, int slen )
{
	char *a, *b, *c, *d;
	int la, lb, lc, ld;
	QARG *qa, *qalist;
	QLYR *nl;

	a = memchr( src,  ',', len );
	b = memchr( src,  '(', len );
	c = memrchr( src, ')', len );

	qalist = NULL;

	// Comma first
	if( a && ( !b || a < b ) )
	{
		la = a - src;
		qa = (QARG *) allocz( sizeof( QARG ) );
		qa->val.s = src;
		qa->len = a - src;


		if( memchr( src, '*', a - src ) )
			qa->type = QARG_TYPE_MULTI;
		else
			qa->type = QARG_TYPE_SERIES;
	}

	return 0;
}







int A_query_parse_args( QRY *q, QLYR *ql, char *src, int slen )
{
	char *s, *op, *cn, *cr;
	int len, lop, lcn, lcr;
	QLYR *qc;

	s = src;
	len = slen;
	lop = lcn = lcr = -1;

	// look for parentheses, front (, next ), last )
	if( ( op = memchr( s, '(', len ) ) )
	{
		// we swat this one, as we cannot want it again
		// we step the pointer to the start of the args,
		// so cr - op will be the length of the arguments
		lop = op - s;
		*op++ = '\0';
	}
	if( ( cn = memchr( s, ')', len ) ) )
		lcn = cn - s;
	if( ( cr = memrchr( s, ')', len ) ) )
	{
		// we swat this one (we cannot want it again) but don't
		// move the pointer
		lcr = cr - s;
		*cr = '\0';
	}

	// make a layer
	qc = mem_new_query_layer( );

	// do we have a function name?
	if( lop > 0 )
	{
		// go look it up
		if( !( qc = query_parse_function( q, s, lop ) ) )
		{
			warn( "Broken query - unrecognised function '%s'.", s );
			return -1;
		}
	}
	

	if( lop < 0 )
	{
		// we have only args, process them and return
		if( lcn >= 0 )
		{
			warn( "Broken query - unmatched ')'." );
			return -1;
		}

		// go get simple args
		return query_parse_args( q, s, len );
	}

	if( lcn < 0 )
	{
		// unbalanced parentheses

		// whine? first?
		warn( "Broken query - unmatched '('." );
		return -1;
	}

	// we don't allow (args), so if lop is zero, fail
	if( lop == 0 )
	{
		warn( "Broken query - no function name before '('." );
		return -1;
	}

	// can we find that function?
	if( !( ql->fn = query_parse_function( q, s, lop ) ) )
	{
		warn( "Broken query - function not found: %s\n", s );
		return -1;
	}


	// do we have simple args inside a function?
	if( lcn == lcr )
	{
		return query_parse_args( q, op, cn - op );
	}

	// OK, we have multiple braces
	// eg function1( f2( foo, bar ), bleugh, f3( eek, org ) )
}

