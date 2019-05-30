/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* pmet/label.c - functions to provide prometheus metrics labels          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


#define _lbl_render( _l )		strbuf_aprintf( b, "%s=\"%s\",", _l->name, _l->val )

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


PMET_LBL *pmet_label_create( char *name, char *val, PMET *item )
{
	PMET_LBL *l = (PMET_LBL *) allocz( sizeof( PMET_LBL ) );

	l->name = str_dup( name, 0 );
	l->val = str_dup( val, 0 );

	if( item )
	{
		l->next = item->labels;
		item->labels = l;
		item->lcount++;
	}

	return l;
}


int pmet_label_common( char *name, char *val )
{
	PMET_LBL *l;

	if( !name || !*name || !val || !*val )
		return -1;

	if( !( l = pmet_label_create( name, val, NULL ) ) )
		return -1;

	l->next = _pmet->common;
	_pmet->common = l;

	return 0;
}

PMET_LBL *pmet_label_clone( PMET_LBL *in, int max )
{
	PMET_LBL *out = NULL, *l;
	int i = 0;

	if( !in )
		return NULL;

	if( max < 0 )
		max = 1000000000;

	while( in && i++ < max )
	{
		l = pmet_label_create( in->name, in->val, NULL );

		l->next = out;
		out = l;

		in = in->next;
	}

	return (PMET_LBL *) mem_reverse_list( out );
}

PMET_LBL **pmet_label_array( char *name, int extra, int count, double *vals )
{
	char valtmp[16];
	PMET_LBL **ret;
	int i;

	ret = (PMET_LBL **) allocz( ( extra + count ) * sizeof( PMET_LBL * ) );

	for( i = 0; i < count; i++ )
	{
		snprintf( valtmp, 16, "%0.6f", vals[i] );
		ret[i] = pmet_label_create( name, valtmp, NULL );
	}

	return ret;
}


