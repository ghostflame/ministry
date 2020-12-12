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
* query/query.c - query main functions                                    *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



void query_add_path( QRY *q, TEL *tel )
{
	QP *qp;
	TEL *t;

	if( tel->leaf )
	{
		qp = mem_new_qrypt( );
		qp->tel = tel;
		qp->next = q->paths;
		q->paths = qp;

		++(q->pcount);
	}
	else
		for( t = tel->child; t; t = t->next )
			query_add_path( q, t );
}



void query_search_tree( QRY *q, int idx, TEL *curr )
{
	int star = 0;
	char *pat;
	TEL *t;

	pat = q->w.wd[idx];

	// hash means everything from here down
	if( !strcmp( pat, "#" ) )
	{
		// but only at the end of the pattern
		// otherwise it means *
		if( idx < q->plast )
			star = 1;
		else
		{
			// ok, end of pattern - scoop up everything
			for( t = curr->child; t; t = t->next )
				query_add_path( q, t );

			return;
		}
	}
	else if( !strcmp( pat, "*" ) )
		star = 1;

	// go check
	for( t = curr->child; t; t = t->next )
		if( star || fnmatch( pat, t->name, 0 ) == 0 )
		{
			if( t->leaf )
			{
				if( idx == q->plast )
					query_add_path( q, t );

				continue;
			}
			else if( idx < q->plast )
				query_search_tree( q, idx + 1, t );
		}
}


int query_search( QRY *q )
{
	char *copy;

	copy = str_copy( q->search, q->slen );
	strwords( &(q->w), copy, q->slen, '.' );

	// last index
	q->plast = q->w.wc - 1;

	query_search_tree( q, 0, ctl->rkv->root );

	free( copy );

	return 0;
}



QRY *query_create( const char *search_string )
{
	QRY *q = (QRY *) allocz( sizeof( QRY ) );

	q->slen   = strlen( search_string );
	q->search = str_copy( search_string, q->slen );
	q->q_when = get_time64( );

	if( query_parse( q ) != 0 )
	{
		free( q->search );
		free( q );
		return NULL;
	}

	return q;
}

void query_free( QRY *q )
{
	double diff;

	diff = (double) ( get_time64( ) - q->q_when );
	diff /= BILLIONF;

	info( "Query fetched %d path%s in %.6fs", q->pcount,
			( q->pcount == 1 ) ? "" : "s", diff );

	free( q->search );

	if( q->paths )
		mem_free_qrypt_list( q->paths );

	free( q );
}


