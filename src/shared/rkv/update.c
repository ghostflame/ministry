/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* rkv/update.c - add points to file                                       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

#define tinfo( fmt, ... )


void rkv_update( RKFL *r, PNT *points, int count )
{
	int64_t of, ts;
	PNT *p, *q;
	int i, j;
	RKBKT *b;
	PNTA *a;

	for( p = points, i = 0; i < count; ++i, ++p )
	{
		// bucket 0 - straight points
		b = r->hdr->buckets;

		ts = p->ts - ( p->ts % b->period );
		of = ( ts / b->period ) % b->count;

		q  = ((PNT *) r->ptrs[0]) + of;
		q->ts = ts;
		q->val = p->val;

		// buckets 1+ - aggregates
		for( j = 1; j < r->hdr->start.buckets; ++j )
		{
			b = r->hdr->buckets + j;

			ts = p->ts - ( p->ts % b->period );
			of = ( ts / b->period ) % b->count;

			a  = ((PNTA *) r->ptrs[j]) + of;

			if( a->ts != ts )
			{
				// set it to just this point
				a->ts    = ts;
				a->count = 1;
				a->sum   = p->val;
				a->min   = p->val;
				a->max   = p->val;
			}
			else
			{
				// add this point in
				++(a->count);
				a->sum += p->val;
				if( a->max < p->val )
					a->max = p->val;
				else if( a->min > p->val )
					a->min = p->val;
			}
		}
	}
}


