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



int metrics_fix_pointer( SSTE *e, void *arg )
{
	METAL *a;

	if( !( a = metrics_get_alist( e->ptr ) ) )
		return -1;

	// ptr is initially set as the list name
	free( e->ptr );
	e->ptr = a;

	return 0;
}


/*
 * Go through each metrics attr lists and sort them
 * Go through the profiles and resolve their attr lists
 */
int metrics_init( void )
{
	METPR *p;
	METMP *m;

	// resolve the attr list names in our profiles
	for( p = _met->profiles; p; p = p->next )
	{
		if( p->default_att
		 && !( p->default_attrs = metrics_get_alist( p->default_att ) ) )
			return -1;

		for( m = p->maps; m; m = m->next )
			if( !( m->list = metrics_get_alist( m->lname ) ) )
				return -2;

		if( !p->paths || string_store_entries( p->paths ) )
			continue;

		// run through the paths entries, which was path:list
		// make sure we can resolve each one of those
		if( string_store_iterator( p->paths, NULL, &metrics_fix_pointer, 0, 0 ) != 0 )
			return -3;
	}

	return 0;
}


