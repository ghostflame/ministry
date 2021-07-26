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
* pmet/counter.c - functions to provide prometheus metrics counters       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



int pmet_counter_render( int64_t mval, BUF *b, PMET *item, PMET_LBL *with )
{
	strbuf_add( b, item->metric->path, item->metric->plen );
	pmet_label_render( b, 2, item->labels, with );
	strbuf_aprintf( b, " %f %ld\n", item->value.dval, mval );

	// flatten the count - we never flatten the value
	lock_pmet( item );

	item->count = 0;

	unlock_pmet( item );

	return 1;
}




int pmet_counter_value( PMET *item, double value, int set )
{
	if( item->type != PMET_TYPE_COUNTER )
		return -1;

	lock_pmet( item );

	item->value.dval += value;
	++(item->count);

	unlock_pmet( item );

	return 0;
}

