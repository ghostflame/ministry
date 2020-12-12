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

