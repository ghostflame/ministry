/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* pmet/summary.c - functions to provide prometheus metrics summaries      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"



int pmet_summary_render( int64_t mval, BUF *b, PMET *item, PMET_LBL *with )
{
	PMET_HIST *s = item->value.summ;
	int i, idx, c;
	double dc;

	// capture the count at this moment
	// from then on, any other values until we 
	// flatten the structure are lost :-/
	lock_pmet( item );
	c = s->count;
	unlock_pmet( item );

	dc = (double) c;
	mem_sort_dlist( s->values, c );

	for( i = 0; i < s->qcount; i++ )
	{
		strbuf_add( b, item->path, item->plen );
		pmet_label_render( b, 3, item->labels, with, s->labels[i] );
		idx = (int) ( dc * s->quantiles[i] );
		strbuf_aprintf( " %f %ld\n", s->values[idx], mval );
	}

	strbuf_aprintf( b, "%s_sum", item->path );
	pmet_label_render( b, 2, item->labels, with );
	strbuf_aprintf( " %f %ld\n", s->sum, mval );

	strbuf_aprintf( b, "%s_count", item->path );
	pmet_label_render( b, 2, item->labels, with );
	strbuf_aprintf( " %ld %ld\n", h->count, mval );

	// flatten it
	lock_pmet( item );

	memset( s->values, 0, s->count * sizeof( double ) );
	s->count = 0;
	s->sum = 0;

	unlock_pmet( item );
}




void __pmet_summary_labels( PMET *item )
{
	PMET_SUMM *s = item->value.summ;
	char *valtmp;
	PMET_LBL *l;
	int i;

	s->labels = (PMET_LBL **) allocz( s->qcount * sizeof( PMET_LBL * ) );
	s->qvals = (char **) allocz( s->qcount * sizeof( char * ) );

	for( i = 0; i < h->bcount; i++ )
	{
		valtmp = (char *) allocz( 12 );
		snprintf( valtmp, 12, "%0.4f", s->quantiles[i] );
		s->qvals[i] = valtmp;
		s->labels[i] = pmet_label_create( "le", s->qvals + i, NULL );
	}
}



int pmet_summary_set( PMET *item, int count, double *vals, int copy )
{
	PMET_SUMM *s = item->value.summ;

	h->qcount = count;

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

	__pmet_summary_labels( item );

	return 0;
}


int pmet_summary_setv( PMET *item, int count, ... )
{
	double *list;
	va_list ap;
	int i, ret;

	list = (double *) allocz( count * sizeof( double ) );

	va_start( ap, count );
	for( i = 0; i < count; i++ )
		list[i] = va_arg( ap, double );
	va_end( ap );

	return pmet_summary_set( item, count, list, 0 );
}


int pmet_summary_make_space( PMET *item, int64_t max )
{
	if( !item )
		return -1;

	item->max = max;
	item->values = (double *) allocz( max * sizeof( double ) );

	return 0;
}


int pmet_summary_value( PMET *item, double value )
{
	PMET_HIST *s = item->value.summ;
	int i, j, ret = 0;

	if( item->type->type != PMET_TYPE_SUMMARY )
		return -1;

	lock_pmet( item )

	if( s->count < s->max )
	{
		s->values[s->count] = value;
		s->sum += value;
		s->count++;
	}
	else
		ret = -1;

	unlock_pmet( item );

	return ret;
}

