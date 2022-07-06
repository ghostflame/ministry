/**************************************************************************
* Copyright 2015 John Denholm                                             *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
*                                                                         *
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
	int i, j = 0;
	va_list ap;

	if( !count && !_proc->pmet->common )
		return;

	strbuf_add( b, "{", 1 );

	// there may be common labels
	for( l = _proc->pmet->common; l; l = l->next, ++j )
		_lbl_render( l );

	// and let's see what we were given
	va_start( ap, count );
	for( i = 0; i < count; ++i )
	{
		list = va_arg( ap, PMET_LBL * );

		for( l = list; l; l = l->next, ++j )
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


PMET_LBL *pmet_label_create( char *name, char *val )
{
	PMET_LBL *l = (PMET_LBL *) mem_perm( sizeof( PMET_LBL ) );

	l->name = str_perm( name, 0 );
	l->val = str_perm( val, 0 );

	return l;
}


// a handy way of creating many labels
PMET_LBL *pmet_label_words( WORDS *w )
{
	PMET_LBL *list, *l;
	int i;

	if( !w )
		return NULL;

	if( w->wc & 0x1 )
	{
		warn( "Odd number of words (%d) given to pmet_label_words - ignoring last word.", w->wc );
		w->wc -= 1;
	}

	for( list = NULL, i = w->wc; i > 0; i -= 2 )
	{
		l = pmet_label_create( w->wd[i-2], w->wd[i-1] );
		l->next = list;
		list = l;
	}

	return list;
}


int pmet_label_apply_to( PMET_LBL *list, PMETM *metric, PMET *item )
{
	PMET_LBL *l = list;
	int i = 1;

	if( !list )
		return 0;

	if( metric && item )
	{
		err( "Apply label to either metric or item, not both." );
		return -1;
	}
	if( !metric && !item )
		return 0;

	while( l->next )
	{
		l = l->next;
		++i;
	}

	if( metric )
	{
		lock_pmetm( metric );

		l->next = metric->labels;
		metric->labels = list;

		unlock_pmetm( metric );
	}
	else
	{
		lock_pmet( item );

		l->next = item->labels;
		item->labels = list;

		unlock_pmet( item );
	}

	return i;
}



int pmet_label_common( char *name, char *val )
{
	PMET_LBL *l;

	if( !name || !*name || !val || !*val )
		return -1;

	if( !( l = pmet_label_create( name, val ) ) )
		return -1;

	l->next = _pmet->common;
	_pmet->common = l;

	return 0;
}

PMET_LBL *pmet_label_clone( PMET_LBL *in, int max, PMET_LBL *except )
{
	PMET_LBL *out = NULL, *l;
	int i = 0;

	if( !in )
		return NULL;

	if( max < 0 )
		max = 1000000000;

	while( in && i++ < max )
	{
		if( except && !strcmp( in->name, except->name ) )
		{
			l = except;
			except = NULL;
		}
		else
			l = pmet_label_create( in->name, in->val );

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

	for( i = 0; i < count; ++i )
	{
		snprintf( valtmp, 16, "%0.6f", vals[i] );
		ret[i] = pmet_label_create( name, valtmp );
	}

	return ret;
}


