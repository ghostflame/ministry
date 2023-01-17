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


//int query_filter_signature( QRY *q, PTL *in, PTL **out, int argc, void **argv )

enum qfilter_marks
{
	QFILTER_VALUE_MIN = 0,
	QFILTER_VALUE_AVG,
	QFILTER_VALUE_MAX,
	QFILTER_VALUE_END
};

enum qfilter_criteria
{
	QFILTER_CRITERIA_ABOVE = 0,
	QFILTER_CRITERIA_BETWEEN,
	QFILTER_CRITERIA_BELOW,
	QFILTER_CRITERIA_END
};


void __query_get_marks( PTL *l, PNT *min, PNT *avg, PNT *max )
{
	int i, j;
	PNT *p;

	p = l->points;
	*min = *max = *p;
	avg->ts  = 0;
	avg->val = 0.0;

	for( i = 0, j = 0; i < l->count; ++i, ++p )
		if( p->ts > 0 )
		{
			if( p->val < min->val )
				*min = *p;

			if( p->val > max->val )
				*max = *p;

			avg->val += p->val;
			++j;
		}

	if( j > 0 )
		avg->val /= (double) j;
}

int __query_filter_series( QRY *q, PTL *in, PTL **out, int mk, int crit, double a, double b )
{
	PNT min, avg, max, *mp;
	PTL *list, *pl, *dead;
	int keep;

	// safety check of those.
	if( crit >= QFILTER_CRITERIA_END || crit < 0
	 || mk   >= QFILTER_VALUE_END     || mk < 0 )
		return -1;

	// final check - b must not be less than a
	if( crit == QFILTER_CRITERIA_BETWEEN && b < a )
		return -2;

	*out = NULL;
	list = in;
	dead = NULL;

	switch( mk )
	{
		case QFILTER_VALUE_MIN:
			mp = &min;
			break;
		case QFILTER_VALUE_MAX:
			mp = &max;
			break;
		case QFILTER_VALUE_AVG:
			mp = &avg;
			break;
		default:
			return -3;
	}

	while( list )
	{
		pl = list;
		list = list->next;
		pl->next = NULL;

		max.ts  = min.ts  = avg.ts  = 0;
		max.val = min.val = avg.val = 0.0;
		__query_get_marks( pl, &min, &avg, &max );

		keep = 0;

		switch( crit )
		{
			case QFILTER_CRITERIA_ABOVE:
				if( mp->val > a )
					keep = 1;
				break;

			case QFILTER_CRITERIA_BELOW:
				if( mp->val < b )
					keep = 1;
				break;

			case QFILTER_CRITERIA_BETWEEN:
				if( mp->val >= a && mp->val <= b )
					keep = 1;
				break;

			default:
				break;
		}

		// keep it or not
		if( keep )
		{
			pl->next = *out;
			*out = pl;
		}
		else
		{
			pl->next = dead;
			dead = pl;
		}
	}

	// delete those points series we filtered out
	if( dead )
		mem_free_ptser_list( dead );

	return 0;
}

int query_filter_min_above( QRY *q, PTL *in, PTL **out, int argc, void **argv )
{
	double a;

	if( argc < 1 )
		return -1;

	a = *((double *) argv[0]);

	return __query_filter_series( q, in, out, QFILTER_VALUE_MIN, QFILTER_CRITERIA_ABOVE, a, 0.0 );
}

int query_filter_min_below( QRY *q, PTL *in, PTL **out, int argc, void **argv )
{
	double a;

	if( argc < 1 )
		return -1;

	a = *((double *) argv[0]);

	return __query_filter_series( q, in, out, QFILTER_VALUE_MIN, QFILTER_CRITERIA_ABOVE, a, 0.0 );
}

int query_filter_min_between( QRY *q, PTL *in, PTL **out, int argc, void **argv )
{
	double a, b;

	if( argc < 2 )
		return -1;

	a = *((double *) argv[0]);
	b = *((double *) argv[1]);

	return __query_filter_series( q, in, out, QFILTER_VALUE_MIN, QFILTER_CRITERIA_ABOVE, a, b );
}

int query_filter_max_above( QRY *q, PTL *in, PTL **out, int argc, void **argv )
{
	double a;

	if( argc < 1 )
		return -1;

	a = *((double *) argv[0]);

	return __query_filter_series( q, in, out, QFILTER_VALUE_MIN, QFILTER_CRITERIA_ABOVE, a, 0.0 );
}

int query_filter_max_below( QRY *q, PTL *in, PTL **out, int argc, void **argv )
{
	double a;

	if( argc < 1 )
		return -1;
	a = *((double *) argv[0]);

	return __query_filter_series( q, in, out, QFILTER_VALUE_MIN, QFILTER_CRITERIA_ABOVE, a, 0.0 );
}

int query_filter_max_between( QRY *q, PTL *in, PTL **out, int argc, void **argv )
{
	double a, b;

	if( argc < 2 )
		return -1;

	a = *((double *) argv[0]);
	b = *((double *) argv[1]);

	return __query_filter_series( q, in, out, QFILTER_VALUE_MIN, QFILTER_CRITERIA_ABOVE, a, b );
}

int query_filter_avg_above( QRY *q, PTL *in, PTL **out, int argc, void **argv )
{
	double a;

	if( argc < 1 )
		return -1;

	a = *((double *) argv[0]);

	return __query_filter_series( q, in, out, QFILTER_VALUE_MIN, QFILTER_CRITERIA_ABOVE, a, 0.0 );
}

int query_filter_avg_below( QRY *q, PTL *in, PTL **out, int argc, void **argv )
{
	double a;

	if( argc < 1 )
		return -1;

	a = *((double *) argv[0]);

	return __query_filter_series( q, in, out, QFILTER_VALUE_MIN, QFILTER_CRITERIA_ABOVE, a, 0.0 );
}

int query_filter_avg_between( QRY *q, PTL *in, PTL **out, int argc, void **argv )
{
	double a, b;

	if( argc < 2 )
		return -1;

	a = *((double *) argv[0]);
	b = *((double *) argv[1]);

	return __query_filter_series( q, in, out, QFILTER_VALUE_MIN, QFILTER_CRITERIA_ABOVE, a, b );
}



