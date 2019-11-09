/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem.c - memory control, free list management                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "metric_filter.h"



HFILT *mem_new_hfilt( void )
{
	HFILT *h = mtype_new( ctl->mem->hfilt );
	return h;
}


void mem_free_hfilt( HFILT **h )
{
	HFILT *hp;

	hp = *h;
	*h = NULL;

	// TODO

	mtype_free( ctl->mem->hfilt, hp );
}



MEMT_CTL *memt_config_defaults( void )
{
	MEMT_CTL *m;

	m = (MEMT_CTL *) allocz( sizeof( MEMT_CTL ) );
	m->hfilt = mem_type_declare( "hfilt", sizeof( HFILT ), MEM_ALLOCSZ_HFILT, 0, 1 );

	return m;
}

