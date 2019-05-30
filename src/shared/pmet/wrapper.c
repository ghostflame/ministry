/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* pmet/wrapper.c - functions exposed to the calling applications          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



int pmet_set( PMET *item, int count, double *vals, int copy )
{
	if( !item )
		return -1;

	switch( item->type->type )
	{
		case PMET_TYPE_SUMMARY:
			return pmet_summary_quantile_set( item, count, vals, copy );

		case PMET_TYPE_HISTOGRAM:
			return pmet_histogram_bucket_set( item, count, vals, copy );

		default:
			err( "Prometheus metric type %s does not have set values like that.",
				item->type->name );
	}

	return -1;
}


int pmet_setv( PMET *item, int count, ... )
{
	double *list;
	va_list ap;
	int i;

	if( !item || !count )
		return -1;

	list = (double *) allocz( count * sizeof( double ) );

	va_start( ap, count );

	for( i = 0; i < count; i++ )
		list[i] = va_arg( ap, double );

	va_end( ap );

	return pmet_set( item, count, list, 0 );
}



int pmet_value( PMET *item, double value, int set )
{
	if( !item )
		return -1;

	return (*(item->type->valu))( item, value, set );
}


int pmet_max_vals( PMET *item, int64_t max )
{
	if( !item )
		return -1;

	if( item->type->type != PMET_TYPE_SUMMARY )
		return -2;

	return pmet_summary_make_space( item, max );
}


PMET *pmet_create( int type, char *path, char *help, int gentype, void *genptr, void *genarg )
{
	return pmet_item_create( type, path, help, gentype, genptr, genarg );
}

PMET *pmet_clone( PMET *item, void *genptr, void *genarg )
{
	return pmet_item_clone( item, genptr, genarg );
}

