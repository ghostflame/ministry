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

	// we have to do this entire thing under lock, or risk
	// inconsitent values
	if( item->gtype == PMET_GEN_NONE )
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

	if( item->gtype == PMET_GEN_NONE )
		unlock_pmet( item );

	return h->bcount + 3;
}


int pmet_histogram_bucket_set( PMET *item, int count, double *vals, int copy )
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
	h->labels = pmet_label_array( "le", 1, h->bcount, h->buckets );

	// an extra label +Inf
	h->labels[h->bcount] = pmet_label_create( "le", "+Inf", NULL );

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

