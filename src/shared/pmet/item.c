/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* pmet/item.c - functions to provide prometheus metrics items             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"




PMET *pmet_item_create( PMETM *metric, PMETS *source, int gentype, void *genptr, void *genarg )
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

	if( !genptr && gentype != PMET_GEN_NONE )
	{
		err( "No promtheus metric generation target pointer." );
		return NULL;
	}

	item = (PMET *) allocz( sizeof( PMET ) );
	item->metric = metric;
	item->source = source;
	item->type = metric->type->type;
	item->gtype = gentype;
	item->gen.in = genptr;
	item->garg = genarg;

	// make sure value contains what we need
	switch( item->type )
	{
		case PMET_TYPE_SUMMARY:
			item->value.summ = (PMET_SUMM *) allocz( sizeof( PMET_SUMM ) );
			break;

		case PMET_TYPE_HISTOGRAM:
			item->value.hist = (PMET_HIST *) allocz( sizeof( PMET_HIST ) );
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
PMET *pmet_item_clone( PMET *item, PMETS *source, void *genptr, void *genarg )
{
	PMET *i;

	if( !item )
		return NULL;

	// create a new one
	i = pmet_item_create( item->metric,
			( source ) ? source : item->source,
			item->gtype,
	        ( genptr ) ? genptr : item->gen.in,
	        ( genarg ) ? genarg : item->garg );

	// recreate the labels, because they are a list
	// and we may add a new label to each of them
	// cloned items
	i->labels = pmet_label_clone( item->labels, -1 );

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


