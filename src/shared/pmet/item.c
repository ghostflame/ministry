/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* pmet/item.c - functions to provide prometheus metrics items             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"




int pmet_item_render( int64_t mval, BUF *b, PMET *item, PMET_LBL *with )
{
	if( item->help )
		strbuf_aprintf( b, "HELP %s %s\n", item->path, item->help );
	strbuf_aprintf( b, "TYPE %s %s\n", item->path, item->type->name );

	return (*(item->type->rndr))( mval, b, item, with );
}


int pmet_item_gen( PMET *item )
{
	if( !item )
		return -1;

	switch( item->gtype )
	{
		case PMET_GEN_IVAL:
			item->value.dval = (double) *(item->gen.iptr);
			break;

		case PMET_GEN_DVAL:
			item->value.dval = *(item->gen.dptr);
			break;

		case PMET_GEN_LLCT:
			item->value.dval = (double) lockless_fetch( item->gen.lockless );
			break;

		case PMET_GEN_FN:
			return (*(item->gen.genfn))( item->garg, &(item->value) );

		default:
			err( "Unknown metric gen type '%d'", item->gtype );
			return -1;
	}

	return 0;
}




PMET *pmet_item_create( int type, char *path, char *help, int gentype, void *genptr, void *genarg )
{
	PMET *item;
	int i;

	if( type < 0 || type >= PMET_TYPE_MAX )
	{
		err( "Invalid prometheus type '%d'", type );
		return NULL;
	}

	if( !path || !*path )
	{
		err( "No prometheus metric path provided." );
		return NULL;
	}

	if( !genptr )
	{
		err( "No promtheus metric generation target pointer." );
		return NULL;
	}

	if( regexec( &(_pmet->path_check), path, 0, NULL, 0 ) )
	{
		err( "Prometheus metric path '%s' is invalid (against regex check).", path );
		return NULL;
	}

	if( gentype < 0 || gentype >= PMET_GEN_MAX )
	{
		err( "Invalid prometheus metric gen type '%d'", gentype );
		return NULL;
	}

	item = (PMET *) allocz( sizeof( PMET ) );
	item->gtype = gentype;
	item->garg = genarg;
	item->path = str_dup( path, 0 );

	// doesn't matter which one we set
	item->gen.dptr = (double *) genptr;

	if( help )
		item->help = str_dup( help, 0 );

	for( i = 0; i < PMET_TYPE_MAX; i++ )
		if( pmet_types[i].type == type )
		{
			item->type = pmet_types + i;
			break;
		}

	// make sure value contains what we need
	switch( type )
	{
		case PMET_TYPE_SUMMARY:
			item->value.summ = (PMET_SUMM *) allocz( sizeof( PMET_SUMM ) );
			break;

		case PMET_TYPE_HISTOGRAM:
			item->value.hist = (PMET_HIST *) allocz( sizeof( PMET_HIST ) );
			break;
	}

	pthread_mutex_init( &(item->lock), NULL );

	return item;
}

// useful for making a set of items with different labels
PMET *pmet_item_clone( PMET *item, void *genptr, void *genarg )
{
	PMET *i;

	if( !item )
		return NULL;

	// create a new one
	i = pmet_item_create( item->type->type,
	        item->path, item->help, item->gtype,
	        ( genptr ) ? genptr : item->gen.dptr,
	        ( genarg ) ? genarg : item->garg );

	// recreate the labels, because they are a list
	// and we may add a new label to each of them
	// cloned items
	i->labels = pmet_label_clone( item->labels, -1 );

	// and clone histogram/summary setup
	switch( item->type->type )
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
