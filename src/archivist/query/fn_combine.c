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


// multiply together *present* values.  If only one present, use that
int query_combine_product( QRY *q, PTL *in, PTL **out, int argc, void **argv )
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
				if( (*out)->points[j].ts == 0 )
				{
					(*out)->points[j].ts  = in->points[j].ts;
					(*out)->points[j].val = in->points[j].val;				
				}
				else
					(*out)->points[j].val *= in->points[j].val;
			}

		in = in->next;
	}

	return 0;
}



int query_combine_min( QRY *q, PTL *in, PTL **out, int argc, void **argv )
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
				if( (*out)->points[j].ts == 0 )
				{
					(*out)->points[j].ts  = in->points[j].ts;
					(*out)->points[j].val = in->points[j].val;				
				}
				else if( in->points[j].val < (*out)->points[j].val )
					(*out)->points[j].val = in->points[j].val;
			}

		in = in->next;
	}

	return 0;
}

int query_combine_max( QRY *q, PTL *in, PTL **out, int argc, void **argv )
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
				if( (*out)->points[j].ts == 0 )
				{
					(*out)->points[j].ts  = in->points[j].ts;
					(*out)->points[j].val = in->points[j].val;				
				}
				else if( in->points[j].val > (*out)->points[j].val )
					(*out)->points[j].val = in->points[j].val;
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

int query_combine_presence( QRY *q, PTL *in, PTL **out, int argc, void **argv )
{
	int segs, *ptct, k, expc;
	register int32_t j;
	int64_t intv, ts;

	if( argc > 0 )
	{
		segs = *((int *) argv[0]);

		if( segs < 1 )
			segs = 1;
		else if( segs > 1024 )
			segs = 1024;
	}
	else
		segs = 1;

	*out = mem_new_ptser( segs );
	// interval per segment
	intv = ( q->end - q->start ) / (int64_t) segs;
	// expected points per segment INTERVAL FROM WHERE??
	expc = ( q->end - q->start ) / in->period;
	// timestamp for the output points
	ts   = q->end - ( intv * ( segs - 1 ) );
	// block of counters to keep counts in
	ptct = (int *) allocz( segs * sizeof( int ) );

	// go through the points, counting them
	for( j = 0, k = 0; j < in->count; ++j )
	{
		if( in->points[j].ts >= (*out)->points[k].ts )
		{
			if( k < (segs - 1) )
				++k;
		}
		++(ptct[k]);
	}

	// then calculate percentages
	for( k = 0; k < segs; ++k, ts += intv )
	{
		(*out)->points[k].ts = ts;

		if( expc > 0 )
			(*out)->points[k].val = (double) ptct[k] / (double) expc;
	}

	free( ptct );
	return 0;
}

