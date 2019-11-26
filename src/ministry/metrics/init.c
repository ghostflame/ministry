/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* metrics/init.c - functions for metrics startup                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



void metrics_init_data( MDATA *m )
{
	// make an entry hash
//	m->entries = (METRY **) allocz( m->hsz * sizeof( METRY * ) );
	m->entries = string_store_create( 0, "tiny", NULL, 0 );

	// make a words struct
	m->wds = (WORDS *) allocz( sizeof( WORDS ) );

	// a metric buffer
	m->buf = (char *) allocz( METR_BUF_SZ );

	// and get our profile
	m->profile = metrics_find_profile( m->profile_name );
}


METAL *metrics_get_alist( char *name )
{
	METAL *m;
	int l;

	l = strlen( name );

	for( m = _met->alists; m; m = m->next )
		if( l == m->nlen && !strncasecmp( m->name, name, l ) )
			return m;

	err( "Could not resolve matrics attribute list for name '%s'", name );

	return NULL;
}


/*
 * Go through each metrics attr lists and sort them
 * Go through the profiles and resolve their attr lists
 */
int metrics_init( void )
{
	METAL *a;
	METPR *p;
	METMP *m;
	SSTE *e;
	int i;

	// resolve the attr list names in our profiles
	for( p = _met->profiles; p; p = p->next )
	{
		if( p->default_att
		 && !( p->default_attrs = metrics_get_alist( p->default_att ) ) )
			return -1;

		for( m = p->maps; m; m = m->next )
			if( !( m->list = metrics_get_alist( m->lname ) ) )
				return -2;

		if( !p->paths || !p->paths->entries )
			continue;

		// run through the paths entries, which was path:list
		// make sure we can resolve each one of those
		string_store_locking( p->paths, 1 );
		for( i = 0; i < p->paths->hsz; ++i )
		{
			for( e = p->paths->hashtable[i]; e; e = e->next )
			{
				if( !( a = metrics_get_alist( e->ptr ) ) )
					return -3;

				// ptr is initially set as the list name
				free( e->ptr );
				e->ptr = a;
			}
		}
		string_store_locking( p->paths, 0 );
	}

	return 0;
}


