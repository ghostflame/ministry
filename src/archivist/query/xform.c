/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
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



