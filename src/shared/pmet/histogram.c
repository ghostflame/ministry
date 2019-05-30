/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* pmet/histogram.c - functions to provide prometheus metrics histograms   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



int pmet_histogram_render( int64_t mval, BUF *b, PMET *item, PMET_LBL *with )
{
	PMET_HIST *h = item->value.hist;
	int i;

	if( item->count == 0 )
		return 0;

	// we have to do this entire thing under lock, or risk
	// inconsitent values
	lock_pmet( item );

	for( i = 0; i < h->bcount; i++ )
	{
		strbuf_aprintf( b, "%s_bucket", item->path );
		pmet_label_render( b, 3, item->labels, with, h->labels[i] );
		strbuf_aprintf( b, " %ld %ld\n", h->counts[i], mval );
	}

	strbuf_aprintf( b, "%s_bucket", item->path );
	pmet_label_render( b, 3, item->labels, with, h->labels[h->bcount] );
	strbuf_aprintf( b, " %ld %ld\n", item->count, mval );

	strbuf_aprintf( b, "%s_sum", item->path );
	pmet_label_render( b, 2, item->labels, with );
	strbuf_aprintf( b, " %f %ld\n", h->sum, mval );

	strbuf_aprintf( b, "%s_count", item->path );
	pmet_label_render( b, 2, item->labels, with );
	strbuf_aprintf( b, " %ld %ld\n", item->count, mval );

	// flatten it

	memset( h->counts, 0, h->bcount * sizeof( int64_t ) );
	item->count = 0;
	h->sum = 0;

	unlock_pmet( item );

	return 0;
}




void __pmet_histogram_labels( PMET *item )
{
	PMET_HIST *h = item->value.hist;
	char *valtmp;
	int i;

	h->labels = (PMET_LBL **) allocz( ( 1 + h->bcount ) * sizeof( PMET_LBL * ) );
	h->bvals = (char **) allocz( h->bcount * sizeof( char * ) );

	for( i = 0; i < h->bcount; i++ )
	{
		valtmp = (char *) allocz( 12 );
		snprintf( valtmp, 12, "%0.4f", h->buckets[i] );
		h->bvals[i] = valtmp;
		h->labels[i] = pmet_label_create( "le", h->bvals + i, NULL );
	}

	h->labels[h->bcount] = pmet_label_create( "le", &(_proc->pmet->plus_inf), NULL );
}



int pmet_histogram_set( PMET *item, int count, double *vals, int copy )
{
	PMET_HIST *h = item->value.hist;

	h->bcount = count;

	if( copy )
	{
		h->buckets = (double *) allocz( count * sizeof( double ) );
		memcpy( h->buckets, vals, count * sizeof( double ) );
	}
	else
	{
		h->buckets = vals;
	}

	// sort them
	mem_sort_dlist( h->buckets, h->bcount );

	h->counts = (int64_t *) allocz( count * sizeof( int64_t ) );

	__pmet_histogram_labels( item );

	return 0;
}



int pmet_histogram_value( PMET *item, double value, int set )
{
	PMET_HIST *h = item->value.hist;
	int i, j;

	if( item->type->type != PMET_TYPE_HISTOGRAM )
		return -1;

	// find the right bucket
	for( j = -1, i = 0; i < h->bcount; i++ )
		if( value < h->buckets[i] )
		{
			j = i;
			break;
		}

	// update under lock
	lock_pmet( item );

	item->count++;
	h->sum += value;

	if( j >= 0 )
		h->counts[j]++;

	unlock_pmet( item );

	return 0;
}

