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

#include "min-monitor.h"


MON *mem_new_monit( void )
{
	MON *m = mtype_new( ctl->mem->monit );

	return m;
}



void mem_free_monit( MON **m )
{
	MON *mp;

	mp = *m;
	*m = NULL;

	memset( mp, 0, sizeof( MON ) );

	mtype_free( ctl->mem->monit, mp );
}



MEMT_CTL *memt_config_defaults( void )
{
	MEMT_CTL *m;

	m = (MEMT_CTL *) mem_perm( sizeof( MEMT_CTL ) );

	m->monit = mem_type_declare( "monit", sizeof( MON ), MEM_ALLOCSZ_MONIT, 0, 1 );

	return m;
}

