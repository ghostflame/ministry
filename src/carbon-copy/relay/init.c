/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* relay/relay.c - handles connections and processes lines                 *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"



int relay_init( void )
{
	RELAY *r;
	TGTL *l;
	WORDS w;
	int i;

	for( r = _relay->rules; r; r = r->next )
	{
		// special case - blackhole
		if( !strcasecmp( r->target_str, "blackhole" ) )
		{
			r->drop = 1;
			continue;
		}

		strwords( &w, r->target_str, strlen( r->target_str ), ',' );

		r->tcount  = w.wc;
		r->targets = (TGTL **) allocz( w.wc * sizeof( TGTL * ) );

		for( i = 0; i < w.wc; ++i )
		{
			if( !( l = target_list_find( w.wd[i] ) ) )
			{
				free( r->targets );
				r->tcount = 0;
				return -1;
			}

			r->targets[i] = l;
		}
	}

	return 0;
}


