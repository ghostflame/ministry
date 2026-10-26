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
* query/defs.c - query data function definitions                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

enum query_parse_outcomes
{
	QPARSE_NO_PARENTHESES = 0,
	QPARSE_FOUND_PARENTHESES,
	QPARSE_MISMATCHED
};


QRFN query_function_defns[] =
{
	// transform one series
	{
		.name  = "scale",
		.fn    = &query_xform_scale,
		.argc  = 1,
		.argm  = 1,
		.types = { QARG_TYPE_DOUBLE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},
	{
		.name  = "offset",
		.fn    = &query_xform_scale,
		.argm  = 1,
		.argc  = 1,
		.types = { QARG_TYPE_DOUBLE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},
	{
		.name  = "abs",
		.fn    = &query_xform_scale,
		.argc  = 0,
		.argm  = 0,
		.types = { QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},
	{
		.name  = "log",
		.fn    = &query_xform_scale,
		.argc  = 1,
		.argm  = 0,
		.types = { QARG_TYPE_INTEGER, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},
	{
		.name  = "derivative",
		.fn    = &query_xform_scale,
		.argc  = 0,
		.argm  = 0,
		.types = { QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},
	{
		.name  = "non_negative_derivative",
		.fn    = &query_xform_scale,
		.argc  = 0,
		.argm  = 0,
		.types = { QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},
	{
		.name  = "integral",
		.fn    = &query_xform_scale,
		.argc  = 0,
		.argm  = 0,
		.types = { QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},
	{
		.name  = "summarize",
		.fn    = &query_xform_summarize,
		.argc  = 2,
		.argm  = 1,
		.types = { QARG_TYPE_INTEGER, QARG_TYPE_INTEGER, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},

	// combine series

	{
		.name  = "sum",
		.fn    = &query_combine_sum,
		.argc  = 0,
		.argm  = 0,
		.types = { QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},
	{
		.name  = "average",
		.fn    = &query_combine_average,		
		.argc  = 0,
		.argm  = 0,
		.types = { QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},
	{
		.name  = "presence",
		.fn    = &query_combine_presence,
		.argc  = 2,
		.argm  = 0,
		.types = { QARG_TYPE_INTEGER, QARG_TYPE_INTEGER, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},

	// filter functions

	{
		.name  = "filter_average_above",
		.fn    = &query_filter_avg_above,
		.argc  = 1,
		.argm  = 1,
		.types = { QARG_TYPE_DOUBLE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},

	{
		.name  = "filter_average_below",
		.fn    = &query_filter_avg_below,
		.argc  = 1,
		.argm  = 1,
		.types = { QARG_TYPE_DOUBLE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},

	{
		.name  = "filter_average_between",
		.fn    = &query_filter_avg_between,
		.argc  = 2,
		.argm  = 2,
		.types = { QARG_TYPE_DOUBLE, QARG_TYPE_DOUBLE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},

	{
		.name  = "filter_max_above",
		.fn    = &query_filter_max_above,
		.argc  = 1,
		.argm  = 1,
		.types = { QARG_TYPE_DOUBLE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},

	{
		.name  = "filter_max_below",
		.fn    = &query_filter_max_below,
		.argc  = 1,
		.argm  = 1,
		.types = { QARG_TYPE_DOUBLE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},

	{
		.name  = "filter_max_between",
		.fn    = &query_filter_max_between,
		.argc  = 2,
		.argm  = 2,
		.types = { QARG_TYPE_DOUBLE, QARG_TYPE_DOUBLE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},

	{
		.name  = "filter_min_above",
		.fn    = &query_filter_min_above,
		.argc  = 1,
		.argm  = 1,
		.types = { QARG_TYPE_DOUBLE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},

	{
		.name  = "filter_min_below",
		.fn    = &query_filter_min_below,
		.argc  = 1,
		.argm  = 1,
		.types = { QARG_TYPE_DOUBLE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},

	{
		.name  = "filter_min_between",
		.fn    = &query_filter_min_between,
		.argc  = 2,
		.argm  = 2,
		.types = { QARG_TYPE_DOUBLE, QARG_TYPE_DOUBLE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},

	{
		.name  = "filter_value",
		.fn    = &query_filter_value,
		.argc  = 4,
		.argm  = 3,
		.types = { QARG_TYPE_STRING, QARG_TYPE_STRING, QARG_TYPE_DOUBLE, QARG_TYPE_DOUBLE }
	},

	// end
	{
		.name  = NULL,
		.fn    = NULL,
		.argc  = 0,
		.argm  = 0,
		.types = { 0 }
	}
};

/*
QPRES *new_query_parse_res( QRY *q, int slen, char *search )
{
	QPRES *r;

	r = (QPRES *) allocz( sizeof( QPRES ) );
	r->qry   = q;
	r->in    = search;
	r->inlen = slen;

	return r;
}

int query_find_parentheses( QPRES *qr )
{
	int l;

	if( ( qr->fpar = memchr( qr->in, '(', qr->inlen ) ) )
	{
		qr->flen = qr->fpar - qr->in;
		l = qr->inlen - qr->flen - 1;

		if( !( qr->bpar = memrchr( qr->fpar + 1, ')', l ) ) )
			return QPARSE_MISMATCHED;

		qr->blen = qr->bpar - qr->in;

		return QPARSE_FOUND_PARENTHESES;
	}

	return QPARSE_NO_PARENTHESES;
}
*/

/*
 *  Parsing strategy - unwrap layers like an onion
 *  	- Search for first parenthesis
 *  	- Search for end parenthesis
 *  	- base to first parenthesis is a function name, look it up
 *  	- inside are arguments, or maybe more functions
 */
/*
int query_parse_functions( QPRES *qr )
{
	QRY *q = qr->qry;
	QRFN *fp;
	QRFC *c;
	int r;

	// parse out to in; all functions take their paths as the first arg
	// that first arg may be another function that 'returns' paths
	// eg sum( filter( "http.servers.*.404s", "average", "above", 20 ) )
	if( ( r = query_find_parentheses( qr ) ) == QPARSE_NO_PARENTHESES )
	{
		return -1;

		// find that name
		for( fp = query_function_defns; fp->name != NULL; ++fp )
			if( qr->flen == fp->nlen && !strncasecmp( fp->name, qr->in, fp->nlen ) )
				break;

		// none-existant?
		if( !fp->name )
			return -1;

		// allocate a structure
		c = (QRFC *) allocz( sizeof( QRFC ) );
		c->fn = fp;
		c->qry = q;

		// push down stack of functions
		c->next = q->fns;
		q->fns  = c;

		// get the args - recurse on the first arg
		// begins at n, length l
		qr = new_query_parse_res( q, q->fpar + 1, q->blen - q->flen );
	}

	return 0;
}

int query_parse( QRY *q )
{
	QPRES *qr;

	qr = new_query_parse_res( q, q->in, q->inlen );

	// parse the input
	query_parse_functions( &qr );


	// go figure out the paths
	query_search( q );

	debug( "Query parse called." );
	return 0;
}
*/

