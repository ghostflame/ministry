/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem.c - memory control, free list management                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "metric_filter.h"

FFILE *mem_new_ffile( void )
{
	return (FFILE *) mtype_new( ctl->mem->ffile );
}

void mem_clean_ffile( FFILE *f )
{
	FFILE *next;

	next = f->next;
	if( f->fname )
		free( f->fname );
	if( f->jo )
		json_object_put( f->jo );

	memset( f, 0, sizeof( FFILE ) );
	f->next = next;
}

void mem_free_ffile( FFILE **f )
{
	FFILE *fp;

	fp = *f;
	*f = NULL;

	mem_clean_ffile( fp );
	mtype_free( ctl->mem->ffile, fp );
}

void mem_free_ffile_list( FFILE *list )
{
	int j = 0;
	FFILE *f;

	for( f = list; f->next; f = f->next, ++j )
		mem_clean_ffile( f );

	mem_clean_ffile( f );

	mtype_free_list( ctl->mem->ffile, j, list, f );
}


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

