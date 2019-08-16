/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* pmet/gauge.c - functions to provide prometheus metrics counters         *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



int pmet_gauge_render( int64_t mval, BUF *b, PMET *item, PMET_LBL *with )
{
	strbuf_add( b, item->metric->path, item->metric->plen );
	pmet_label_render( b, 2, item->labels, with );
	strbuf_aprintf( b, " %f %ld\n", item->value.dval, mval );

	// flatten it
	lock_pmet( item );

	item->count = 0;

	unlock_pmet( item );

	return 1;
}




int pmet_gauge_value( PMET *item, double value, int set )
{
	if( item->type != PMET_TYPE_GAUGE )
		return -1;

	lock_pmet( item );

	if( set )
		item->value.dval  = value;
	else
		item->value.dval += value;

	++(item->count);

	unlock_pmet( item );

	return 0;
}

