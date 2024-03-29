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
* pmet/summary.c - functions to provide prometheus metrics summaries      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



int pmet_summary_render( int64_t mval, BUF *b, PMET *item, PMET_LBL *with )
{
	PMET_SUMM *s = item->value.summ;
	PMETM *met = item->metric;
	int i, idx, c;
	double dc;

	// capture the count at this moment
	// from then on, any other values until we 
	// flatten the structure are lost :-/
	lock_pmet( item );
	c = item->count;
	unlock_pmet( item );

	dc = (double) c;
	mem_sort_dlist( s->values, c );

	for( i = 0; i < s->qcount; ++i )
	{
		strbuf_add( b, met->path, met->plen );
		pmet_label_render( b, 3, item->labels, with, s->labels[i] );
		idx = (int) ( dc * s->quantiles[i] );
		strbuf_aprintf( b, " %f %ld\n", s->values[idx], mval );
	}

	strbuf_aprintf( b, "%s_sum", met->path );
	pmet_label_render( b, 2, item->labels, with );
	strbuf_aprintf( b, " %f %ld\n", s->sum, mval );

	strbuf_aprintf( b, "%s_count", met->path );
	pmet_label_render( b, 2, item->labels, with );
	strbuf_aprintf( b, " %ld %ld\n", item->count, mval );

	// flatten it
	lock_pmet( item );

	memset( s->values, 0, item->count * sizeof( double ) );
	item->count = 0;
	s->sum = 0;

	unlock_pmet( item );

	return s->qcount + 2;
}




int pmet_summary_quantile_set( PMET *item, int count, double *vals, int copy )
{
	PMET_SUMM *s = item->value.summ;

	// can be called from item clone on an item
	// that isn't set up yet
	// that doesn't check the return code for this very reason
	if( !count )
		return -1;

	s->qcount = count;

	if( copy )
	{
		s->quantiles = (double *) allocz( count * sizeof( double ) );
		memcpy( s->quantiles, vals, count * sizeof( double ) );
	}
	else
	{
		s->quantiles = vals;
	}

	// sort them
	mem_sort_dlist( s->quantiles, s->qcount );

	s->labels = pmet_label_array( "quantile", 0, s->qcount, s->quantiles );

	return 0;
}


int pmet_summary_make_space( PMET *item, int64_t max )
{
	PMET_SUMM *s;

	if( !item || max <= 0 )
		return -1;

	s = item->value.summ;

	s->max = max;
	s->values = (double *) allocz( max * sizeof( double ) );

	return 0;
}


int pmet_summary_value( PMET *item, double value, int set )
{
	PMET_SUMM *s = item->value.summ;
	int ret = 0;

	if( item->type != PMET_TYPE_SUMMARY )
		return -1;

	lock_pmet( item );

	if( item->count < s->max )
	{
		s->values[item->count] = value;
		s->sum += value;
		++(item->count);
	}
	else
		ret = -1;

	unlock_pmet( item );

	return ret;
}

