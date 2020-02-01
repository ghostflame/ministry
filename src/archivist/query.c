/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* query.c - query functions                                               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "archivist.h"

QRY_CTL *_qry = NULL;



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

	query_search_tree( q, 0, ctl->tree->root );

	free( copy );

	return 0;
}



QRY *query_create( char *search_string )
{
	QRY *q = (QRY *) allocz( sizeof( QRY ) );

	q->search = str_copy( search_string, 0 );
	q->slen   = strlen( q->search );

	return q;
}

void query_free( QRY *q )
{
	return;
}



QRY_CTL *query_config_defaults( void )
{
	_qry = (QRY_CTL *) allocz( sizeof( QRY_CTL ) );

	_qry->default_timespan = DEFAULT_QUERY_TIMESPAN;
	_qry->max_paths = DEFAULT_QUERY_MAX_PATHS;

	return _qry;
}


int query_config_line( AVP *av )
{
	return -1;
}


