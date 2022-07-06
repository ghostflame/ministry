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
* query/xform.c - query data functions - transforms                       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

/*
 * Typically in this file, we don't touch **out, and just do work on *in
 */

int query_xform_scale( QRY *q, PTL *in, PTL **out, int argc, void **argv )
{
	register double scale;
	register int j;

	scale = *((double *) argv[0]);

	for( j = 0; j < in->count; ++j )
		in->points[j].val *= scale;

	return 0;
}


int query_xform_offset( QRY *q, PTL *in, PTL **out, int argc, void **argv )
{
	register double offs;
	register int j;

	offs = *((double *) argv[0]);

	for( j = 0; j < in->count; ++j )
		in->points[j].val += offs;

	return 0;
}

int query_xform_abs( QRY *q, PTL *in, PTL **out, int argc, void **argv )
{
	register int j;

	for( j = 0; j < in->count; ++j )
		in->points[j].val = fabs( in->points[j].val );

	return 0;
}

int query_xform_log( QRY *q, PTL *in, PTL **out, int argc, void **argv )
{
	register int j;
	int base;

	if( argc > 0 )
		base = *((int *) argv[0]);
	else
		base = 1;

	switch( base )
	{
		case 2:
			for( j = 0; j < in->count; ++j )
				in->points[j].val = log2( in->points[j].val );
			break;
		case 10:
			for( j = 0; j < in->count; ++j )
				in->points[j].val = log10( in->points[j].val );
			break;
		default:
			for( j = 0; j < in->count; ++j )
				in->points[j].val = log( in->points[j].val );
			break;
	}

	return 0;
}

int query_xform_deriv( QRY *q, PTL *in, PTL **out, int argc, void **argv )
{
	register int j;
	int64_t tdiff;
	PNT p, r;

	p = in->points[0];

	for( j = 1; j < in->count; ++j )
	{
		r = in->points[j];

		tdiff = r.ts - p.ts;
		if( tdiff == 0 )
			in->points[j].val = 0.0;
		else
			in->points[j].val = ( r.val - p.val ) / ( ( (double) tdiff ) / MILLIONF );

		p = r;
	}

	return 0;
}

int query_xform_nn_deriv( QRY *q, PTL *in, PTL **out, int argc, void **argv )
{
	register int j;
	int64_t tdiff;
	PNT p, r;

	p = in->points[0];

	for( j = 1; j < in->count; ++j )
	{
		r = in->points[j];

		tdiff = r.ts - p.ts;
		if( tdiff == 0 || p.val > r.val )
			in->points[j].val = 0.0;
		else
			in->points[j].val = ( r.val - p.val ) / ( ( (double) tdiff ) / MILLIONF );

		p = r;
	}

	return 0;
}


int query_xform_integral( QRY *q, PTL *in, PTL **out, int argc, void **argv )
{
	register int j;
	double sum;

	for( sum = 0.0, j = 1; j < in->count; ++j )
	{
		sum += in->points[j].val;
		in->points[j].val = sum;
	}

	return 0;
}

// we expect the offset to be parsed, in nsec, where going backwards is a
// negative offset.
int query_xform_timeshift( QRY *q, PTL *in, PTL **out, int argc, void **argv )
{
	register int j;
	int64_t offs;

	if( argc > 0 )
	{
		offs = *((int64_t *) argv[0]);

		for( j = 0; j < in->count; ++j )
			in->points[j].ts += offs;
	}

	return 0;
}

enum {
	XFORM_OP_SUM = 0,
	XFORM_OP_MEAN,
	XFORM_OP_MIN,
	XFORM_OP_MAX,
	XFORM_OP_END
};


#define _xform_offset( _q, _in, _j, _per )		_in->points[_j].ts + ( ( _in->points[_j].ts - _q->start ) % _per )
#define _xform_index( _off, _fir, _per )		(int) ( ( _off - _fir ) / _per )

#define _xform_ts( _q, _in, _j, _fir, _per )	off = _xform_offset( _q, _in, _j, _per ); \
												if( off > _q->end ) break; \
												k = _xform_index( off, _fir, _per )


void _query_xform_op_sum( QRY *q, PTL *in, PTL *out, int64_t first, int64_t period )
{
	register int j, k;
	int64_t off;

	for( j = 0; j < in->count; ++j )
	{
		// set off and k
		_xform_ts( q, in, j, first, period );

		// just do sum for now
		out->points[k].ts = off;
		out->points[k].val += in->points[j].val;
	}
}

void _query_xform_op_min( QRY *q, PTL *in, PTL *out, int64_t first, int64_t period )
{
	register int j, k;
	int64_t off;

	for( j = 0; j < in->count; ++j )
	{
		// set off and k
		_xform_ts( q, in, j, first, period );

		// set if unset or lower
		if( out->points[k].ts == 0 || out->points[k].val > in->points[j].val )
		{
			out->points[k].ts = off;
			out->points[k].val = in->points[j].val;
		}
	}
}

void _query_xform_op_max( QRY *q, PTL *in, PTL *out, int64_t first, int64_t period )
{
	register int j, k;
	int64_t off;

	for( j = 0; j < in->count; ++j )
	{
		// set off and k
		_xform_ts( q, in, j, first, period );

		// set if unset or lower
		if( out->points[k].ts == 0 || out->points[k].val < in->points[j].val )
		{
			out->points[k].ts = off;
			out->points[k].val = in->points[j].val;
		}
	}
}


void _query_xform_op_mean( QRY *q, PTL *in, PTL *out, int64_t first, int64_t period )
{
	register int j, k;
	int64_t off;
	int *counts;

	counts = (int *) allocz( out->count * sizeof( int ) );

	for( j = 0; j < in->count; ++j )
	{
		// set off and k
		_xform_ts( q, in, j, first, period );

		out->points[k].ts = off;
		out->points[k].val += in->points[j].val;
		counts[k]++;
	}

	for( k = 0; k < out->count; ++k )
		if( counts[k] > 0 )
			out->points[k].val /= (double) counts[k];

	free( counts );
}


// we need a period, in nsec
int query_xform_summarize( QRY *q, PTL *in, PTL **out, int argc, void **argv )
{
	int64_t period, start;
	int32_t nct, op;

	if( argc > 0 )
		period = *((int64_t *) argv[0]);
	else
		return 0;

	if( argc > 1 )
		op = *((int32_t *) argv[1]);
	else
		op = XFORM_OP_SUM;

	if( !period )
		return 0;

	nct  = (int32_t) ( ( q->end - q->start ) / period );
	*out = mem_new_ptser( nct );

	// figure out the first summarization point
	start = q->start;
	if( ( start % period ) > 0 )
		start = q->start + ( period - ( q->start % period ) );

	switch( op )
	{
		case XFORM_OP_SUM:
			_query_xform_op_sum( q, in, *out, start, period );
			break;
		case XFORM_OP_MEAN:
			_query_xform_op_mean( q, in, *out, start, period );
			break;
		case XFORM_OP_MIN:
			_query_xform_op_min( q, in, *out, start, period );
			break;
		case XFORM_OP_MAX:
			_query_xform_op_max( q, in, *out, start, period );
			break;
	}

	return 0;
}


