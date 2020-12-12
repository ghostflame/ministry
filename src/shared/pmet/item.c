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
* pmet/item.c - functions to provide prometheus metrics items             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"




PMET *pmet_item_create( PMETM *metric, PMETS *source, int gentype, void *genptr, pmet_gen_fn *fp, void *genarg )
{
	PMET *item;

	if( !metric )
	{
		err( "No metric given to connect the new item to." );
		return NULL;
	}

	if( gentype < 0 || gentype >= PMET_GEN_MAX )
	{
		err( "Invalid prometheus metric gen type '%d'", gentype );
		return NULL;
	}

	if( gentype == PMET_GEN_FN )
	{
		if( !fp )
		{
			err( "No prometheus metric generation function given." );
			return NULL;
		}
	}
	else if( gentype != PMET_GEN_NONE )
	{
		if( !genptr )
		{
			err( "No promtheus metric generation target pointer." );
			return NULL;
		}
	}

	item = (PMET *) mem_perm( sizeof( PMET ) );
	item->metric = metric;
	item->source = source;
	item->type = metric->type->type;
	item->gtype = gentype;
	item->garg = genarg;
	if( fp )
		item->gen.genfn = fp;
	else
		item->gen.in = genptr;

	// make sure value contains what we need
	switch( item->type )
	{
		case PMET_TYPE_SUMMARY:
			item->value.summ = (PMET_SUMM *) mem_perm( sizeof( PMET_SUMM ) );
			break;

		case PMET_TYPE_HISTOGRAM:
			item->value.hist = (PMET_HIST *) mem_perm( sizeof( PMET_HIST ) );
			break;
	}

	pthread_mutex_init( &(item->lock), NULL );

	lock_pmetm( metric );

	item->next = metric->items;
	metric->items = item;

	unlock_pmetm( metric );

	return item;
}



// useful for making a set of items with different labels
PMET *pmet_item_clone( PMET *item, PMETS *source, void *genptr, pmet_gen_fn *fp, void *genarg, PMET_LBL *lbl )
{
	void *ga;
	PMET *i;

	if( !item )
		return NULL;

	if( fp )
		ga = NULL;
	else
		ga = ( genptr ) ? genptr : item->gen.in;

	// create a new one
	i = pmet_item_create( item->metric,
			( source ) ? source : item->source,
			item->gtype, ga, fp,
	        ( genarg ) ? genarg : item->garg );

	// recreate the labels, because they are a list
	// and we may add a new label to each of them
	// cloned items
	// the last arg is a way of varying the labels by one
	i->labels = pmet_label_clone( item->labels, -1, lbl );

	// and clone histogram/summary setup
	switch( item->type )
	{
		case PMET_TYPE_SUMMARY:
			pmet_summary_make_space( i, item->value.summ->max );
			pmet_summary_quantile_set( i, item->value.summ->qcount,
			        item->value.summ->quantiles, 1 );
			break;

		case PMET_TYPE_HISTOGRAM:
			pmet_histogram_bucket_set( i, item->value.hist->bcount,
			        item->value.hist->buckets, 1 );
			break;
	}

	return i;
}



