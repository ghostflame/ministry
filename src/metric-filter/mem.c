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

	if( !h->path )
		h->path = strbuf( MAX_PATH_SZ );

	return h;
}


void mem_free_hfilt( HFILT **h )
{
	HFILT *hp;
	FILT *f;
	BUF *b;

	hp = *h;
	*h = NULL;

	b = hp->path;
	strbuf_empty( b );

	while( hp->filters )
	{
		f = hp->filters;
		hp->filters = f->next;
		free( f );
	}

	memset( hp, 0, sizeof( HFILT ) );

	hp->path = b;

	mtype_free( ctl->mem->hfilt, hp );
}



MEMT_CTL *memt_config_defaults( void )
{
	MEMT_CTL *m;

	m = (MEMT_CTL *) mem_perm( sizeof( MEMT_CTL ) );
	m->hfilt = mem_type_declare( "hfilt", sizeof( HFILT ), MEM_ALLOCSZ_HFILT, 0, 1 );

	return m;
}

