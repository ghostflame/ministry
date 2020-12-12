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


