/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* pmet/label.c - functions to provide prometheus metrics labels          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


#define _lbl_render( _l )		strbuf_aprintf( b, "%s=\"%s\",", _l->name, *(_l->valptr) )

void pmet_label_render( BUF *b, int count, ... )
{
	PMET_LBL *l, *list;
	va_list ap;
	int i, j;

	if( !count && !_proc->pmet->common )
		return;

	strbuf_add( b, "{", 1 );

	// there may be common labels
	for( l = _proc->pmet->common; l; l = l->next )
		_lbl_render( l );

	// and let's see what we were given
	va_start( ap, count );
	for( j = 0, i = 0; i < count; i++ )
	{
		list = va_arg( ap, PMET_LBL * );

		for( l = list; l; l = l->next, j++ )
			_lbl_render( l );
	}
	va_end( ap );

	strbuf_chop( b );

	// slight hack here - if we have any labels at all (our pointers
	// count all have been null), then we want the {} around them, and
	// the chop will have removed the trailing comma
	// if we have none, then the chop will have removed the first {
	// and we are done.  So only add the close if we had some labels
	if( j )
		strbuf_add( b, "}", 1 );
}

#undef _lbl_render


PMET_LBL *pmet_label_create( char *name, char **valptr, PMET *item )
{
	PMET_LBL *l = (PMET_LBL *) allocz( sizeof( PMET_LBL ) );

	l->name = str_dup( name, 0 );
	l->valptr = valptr;

	if( item )
	{
		l->next = item->labels;
		item->labels = l;
		item->lcount++;
	}

	return l;
}


