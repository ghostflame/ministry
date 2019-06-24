/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* pmet/metric.c - functions to provide prometheus metrics                 *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


int pmet_metric_render( int64_t mval, BUF *b, PMETM *metric )
{
	PMET *i;

	if( !metric->items )
		return -1;

	if( metric->help )
		strbuf_aprintf( b, "HELP %s %s\n", metric->path, metric->help );
	strbuf_aprintf( b, "TYPE %s %s\n", metric->path, metric->type->name );

	for( i = metric->items; i; i = i->next )
		if( pmets_enabled( i->source ) )
			i->source->last_ct += (*(i->metric->type->rndr))( mval, b, i, metric->labels );

	strbuf_add( b, "\n", 1 );

	return 0;
}


int pmet_metric_gen( int64_t mval, PMETM *metric )
{
	PMET *item;

	if( !metric )
		return -1;

	//debug( "Generating metric %s.", metric->path );

	for( item = metric->items; item; item = item->next )
	{
		if( !pmets_enabled( item->source ) )
			continue;

		switch( item->gtype )
		{
			case PMET_GEN_NONE:
				break;

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
				(*(item->gen.genfn))( mval, item->garg, &(item->value.dval) );
				break;

			default:
				err( "Unknown metric gen type '%d'", item->gtype );
				break;
		}
	}

	return 0;
}



PMETM *pmet_metric_create( int type, char *path, char *help )
{
	PMETM *met;
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

	if( regexec( &(_pmet->path_check), path, 0, NULL, 0 ) )
	{
		err( "Prometheus metric path '%s' is invalid (against regex check).", path );
		return NULL;
	}

	met = (PMETM *) allocz( sizeof( PMETM ) );
	met->plen = strlen( path );
	met->path = str_dup( path, met->plen );

	if( help )
		met->help = str_dup( help, 0 );

	for( i = 0; i < PMET_TYPE_MAX; i++ )
		if( pmet_types[i].type == type )
		{
			met->type = pmet_types + i;
			break;
		}

	pthread_mutex_init( &(met->lock), NULL );

	pmet_genlock( );

	met->next = _pmet->metrics;
	_pmet->metrics = met;

	pmet_genunlock( );

	return met;
}


PMETM *pmet_metric_find( char *name )
{
	SSTE *sse;

	if( !( sse = string_store_look( _pmet->metlookup, name, strlen( name ), 0 ) ) )
		return NULL;

	return (PMETM *) sse->ptr;
}

