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
* pmet/wrapper.c - functions exposed to the calling applications          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



int pmet_set( PMET *item, int count, double *vals, int copy )
{
	if( !item )
		return -1;

	switch( item->type )
	{
		case PMET_TYPE_SUMMARY:
			return pmet_summary_quantile_set( item, count, vals, copy );

		case PMET_TYPE_HISTOGRAM:
			return pmet_histogram_bucket_set( item, count, vals, copy );

		default:
			err( "Prometheus metric type %s does not have set values like that.",
				item->metric->type->name );
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

	for( i = 0; i < count; ++i )
		list[i] = va_arg( ap, double );

	va_end( ap );

	return pmet_set( item, count, list, 0 );
}





int pmet_max_vals( PMET *item, int64_t max )
{
	if( !item )
		return -1;

	if( item->type != PMET_TYPE_SUMMARY )
		return -2;

	return pmet_summary_make_space( item, max );
}


PMET *pmet_create_gen( PMETM *metric, PMETS *source, int gentype, void *genptr, pmet_gen_fn *fp, void *genarg )
{
	return pmet_item_create( metric, source, gentype, genptr, fp, genarg );
}

PMET *pmet_create_name( char *metric, char *source, int gentype, void *genptr, pmet_gen_fn *fp, void *genarg )
{
	SSTE *sse1, *sse2;

	if( !( sse1 = string_store_look( _pmet->metlookup, metric, strlen( metric ), 0 ) ) )
	{
		err( "Metric '%s' not known.", metric );
		return NULL;
	}

	if( !( sse2 = string_store_look( _pmet->srclookup, source, strlen( source ), 0 ) ) )
	{
		err( "Source '%s' not known.", source );
		return NULL;
	}

	return pmet_item_create( (PMETM *) sse1->ptr, (PMETS *) sse2->ptr, gentype, genptr, fp, genarg );
}

PMET *pmet_create( PMETM *metric, PMETS *source )
{
	return pmet_item_create( metric, source, PMET_GEN_NONE, NULL, NULL, NULL );
}

PMET *pmet_clone_gen( PMET *item, PMETS *source, void *genptr, pmet_gen_fn *fp, void *genarg )
{
	return pmet_item_clone( item, source, genptr, fp, genarg, NULL );
}

PMET *pmet_clone( PMET *item )
{
	return pmet_item_clone( item, NULL, NULL, NULL, NULL, NULL );
}

PMET *pmet_clone_vary( PMET *item, PMET_LBL *lbl )
{
	return pmet_item_clone( item, NULL, NULL, NULL, NULL, lbl );
}


PMETM *pmet_new( int type, char *path, char *help )
{
	return pmet_metric_create( type, path, help );
}




int pmet_value_set( PMET *item, double value, int set )
{
	if( !item )
	{
		err( "No item to set value on." );
		return -1;
	}

	//debug( "pm value set: %s -> %f (%d)", item->path, value, set );

	return (*(item->metric->type->valu))( item, value, set );
}

int pmet_value( PMET *item, double value )
{
	if( !item )
	{
		err( "No item to write value to." );
		return -1;
	}

	//debug( "pm value: %s -> %f", item->path, value );

	return (*(item->metric->type->valu))( item, value, 0 );
}

