/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* pmet/counter.c - functions to provide prometheus metrics counters       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



int pmet_counter_render( int64_t mval, BUF *b, PMET *item, PMET_LBL *with )
{
	strbuf_add( b, item->path, item->plen );
	pmet_label_render( b, 2, item->labels, with );
	strbuf_aprintf( b, " %f %ld\n", item->value.dval, mval );

	// flatten it
	lock_pmet( item );

	item->count = 0;
	item->value.dval = 0;

	unlock_pmet( item );

	return 0;
}




int pmet_counter_value( PMET *item, double value, int set )
{
	if( item->type->type != PMET_TYPE_COUNTER )
		return -1;

	lock_pmet( item );

	item->value.dval += value;
	item->count++;

	unlock_pmet( item );

	return 0;
}

