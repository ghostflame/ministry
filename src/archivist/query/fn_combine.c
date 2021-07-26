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
* query/comvbine.c - query data functions - combine                       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


// These functions iterate across the linked list of in - order may matter!
// they create an out series

int query_combine_sum( QRY *q, PTL *in, PTL **out, int argc, void **argv )
{
	register int32_t j;
	int32_t isize;

	isize = in->count;

	*out = mem_new_ptser( isize );

	while( in )
	{
		for( j = 0; j < in->count && j < isize; ++j )
			if( in->points[j].ts > 0 )
			{
				(*out)->points[j].ts   = in->points[j].ts;
				(*out)->points[j].val += in->points[j].val;
			}

		in = in->next;
	}

	return 0;
}

int query_combine_average( QRY *q, PTL *in, PTL **out, int argc, void **argv )
{
	register int32_t j;
	double ctr;
	PTL *l;

	query_combine_sum( q, in, out, argc, argv );

	ctr = 0.0;
	for( l = in; l; l = l->next )
		ctr += 1.0;

	for( j = 0; j < (*out)->count; ++j )
		(*out)->points[j].val /= ctr;

	return 0;
}

