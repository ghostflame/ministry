/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* metrics/sort.c - sorting functions for handling metrics                 *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


int metrics_cmp_attrs( const void *ap1, const void *ap2 )
{
	METAT *m1 = *((METAT **) ap1);
	METAT *m2 = *((METAT **) ap2);

	return ( m1->order < m2->order ) ? -1 : ( m1->order == m2->order ) ? 0 : 1;
}

int metrics_cmp_maps( const void *ap1, const void *ap2 )
{
	METMP *m1 = *((METMP **) ap1);
	METMP *m2 = *((METMP **) ap2);

	return ( m1->id < m2->id ) ? -1 : ( m1->id == m2->id ) ? 0 : 1;
}


void metrics_sort_attrs( METAL *a )
{
	int o = 0;
	METAT *m;

	// sort them
	mem_sort_list( (void **) &(a->ats), a->atct, metrics_cmp_attrs );

	// normalise the ordering
	for( m = a->ats; m; m = m->next )
		m->order = o++;
}




METPR *metrics_find_profile( char *name )
{
	METPR *pr;

	// go looking for a match if we have a name
	if( name )
	{
		for( pr = _met->profiles; pr; pr = pr->next )
			if( !strcasecmp( name, pr->name ) )
				return pr;
	}

	// ok, no match or no name
	for( pr = _met->profiles; pr; pr = pr->next )
		if( pr->is_default )
			return pr;

	// ugh, just default to the first one
	return _met->profiles;
}




METRY *metrics_find_entry( MDATA *m, char *str, int len )
{
	SSTE *se;

	if( ( se = string_store_look( m->entries, str, len, 0 ) ) )
		return (METRY *) se->ptr;

	return NULL;
}


