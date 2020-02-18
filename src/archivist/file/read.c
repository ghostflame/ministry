/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* file/read.c - read points from file                                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

#define frb_loop0( _p )			for( i = q->oa[0]; i < q->ob[0]; ++i, ++j, ++_p, ts += b->period )
#define frb_loop1( _p )			for( i = 0; i < q->ob[1]; ++i, ++j, ++_p, ts += b->period )

#define frb_setpt( _p, _d )		if( _p->ts == ts ) { q->points[j].ts = ts; q->points[j].val = _d; }

#define frb_setupP( )			j = 0; ts = q->first; parr = (PNT *) r->ptrs[0]; p = parr + q->oa[0]
#define frb_setupA( )			j = 0; ts = q->first; aarr = (PNTA *) r->ptrs[q->bkt]; a = aarr + q->oa[0]

#define frb_check1P( )			if( q->ob[1] == 0 ) { return; } p = parr
#define frb_check1A( )			if( q->ob[1] == 0 ) { return; } a = aarr


void file_read_blocks_points( RKFL *r, RKBKT *b, RKQR *q )
{
	int64_t ts, i, j;
	PNT *parr, *p;

	frb_setupP( );

	frb_loop0( p )
	{
		frb_setpt( p, p->val );
	}

	frb_check1P( );

	frb_loop1( p )
	{
		frb_setpt( p, p->val );
	}
}

void file_read_blocks_mean( RKFL *r, RKBKT *b, RKQR *q )
{
	int64_t ts, i, j;
	PNTA *aarr, *a;

	frb_setupA( );

	frb_loop0( a )
	{
		frb_setpt( a, ( a->sum / (double) a->count ) );
	}

	frb_check1A( );

	frb_loop1( a )
	{
		frb_setpt( a, ( a->sum / (double) a->count ) );
	}
}

void file_read_blocks_count( RKFL *r, RKBKT *b, RKQR *q )
{
	int64_t ts, i, j;
	PNTA *aarr, *a;

	frb_setupA( );

	frb_loop0( a )
	{
		frb_setpt( a, ( (double) a->count ) );
	}

	frb_check1A( );

	frb_loop1( a )
	{
		frb_setpt( a, ( (double) a->count ) );
	}
}


void file_read_blocks_sum( RKFL *r, RKBKT *b, RKQR *q )
{
	int64_t ts, i, j;
	PNTA *aarr, *a;

	frb_setupA( );

	frb_loop0( a )
	{
		frb_setpt( a, a->sum );
	}

	frb_check1A( );

	frb_loop1( a )
	{
		frb_setpt( a, a->sum );
	}
}


void file_read_blocks_min( RKFL *r, RKBKT *b, RKQR *q )
{
	int64_t ts, i, j;
	PNTA *aarr, *a;

	frb_setupA( );

	frb_loop0( a )
	{
		frb_setpt( a, a->min );
	}

	frb_check1A( );

	frb_loop1( a )
	{
		frb_setpt( a, a->min );
	}
}


void file_read_blocks_max( RKFL *r, RKBKT *b, RKQR *q )
{
	int64_t ts, i, j;
	PNTA *aarr, *a;

	frb_setupA( );

	frb_loop0( a )
	{
		frb_setpt( a, a->max );
	}

	frb_check1A( );

	frb_loop1( a )
	{
		frb_setpt( a, a->max );
	}
}


void file_read_blocks_spread( RKFL *r, RKBKT *b, RKQR *q )
{
	int64_t ts, i, j;
	PNTA *aarr, *a;

	frb_setupA( );

	frb_loop0( a )
	{
		frb_setpt( a, ( a->max - a->min ) );
	}

	frb_check1A( );

	frb_loop1( a )
	{
		frb_setpt( a, ( a->max - a->min ) );
	}
}


void file_read_blocks_middle( RKFL *r, RKBKT *b, RKQR *q )
{
	int64_t ts, i, j;
	PNTA *aarr, *a;

	frb_setupA( );

	frb_loop0( a )
	{
		frb_setpt( a, ( ( a->max + a->min ) / 2.0 ) );
	}

	frb_check1A( );

	frb_loop1( a )
	{
		frb_setpt( a, ( ( a->max + a->min ) / 2.0 ) );
	}
}


file_rd_fn *file_reader_functions[FILE_VAL_METR_END] =
{
	&file_read_blocks_mean,
	&file_read_blocks_count,
	&file_read_blocks_sum,
	&file_read_blocks_min,
	&file_read_blocks_max,
	&file_read_blocks_spread,
	&file_read_blocks_middle
};



int file_choose_bucket( RKFL *r, int64_t from, int64_t to )
{
	int64_t now, oldest;
	RKBKT *b;
	int i;

	now = _proc->curr_usec;

	for( i = 0; i < r->hdr->start.buckets; ++i )
	{
		b = r->hdr->buckets + i;
		oldest = now - ( b->period * b->count );

		if( oldest < from )
			return i;
	}

	// no matches?  pick the last bucket
	return i - 1;
}



// we assume query struct is correctly curated
void file_read( RKFL *r, RKQR *qry )
{
	int64_t x, y;
	RKBKT *b;

	if( !r->map && file_open( r ) != 0 )
		return;

	qry->bkt = file_choose_bucket( r, qry->from, qry->to );
	b = r->hdr->buckets + qry->bkt;

	// correct the timestamps to be bucket-aligned
	qry->first = qry->from - ( qry->from % b->period );
	qry->last  = qry->to   - ( qry->to   % b->period );

	x = ( qry->first / b->period ) % b->count;
	y = ( qry->last  / b->period ) % b->count;

	// is it in one block or two
	if( x < y )
	{
		qry->count = (int) ( y - x );
		qry->oa[0] = x;
		qry->ob[0] = y;
	}
	else
	{
		qry->count = (int) y + (int) ( b->count - x );
		qry->oa[0] = x;
		qry->ob[0] = b->count;
		qry->oa[1] = 0;
		qry->ob[1] = y;
	}

	//debug( "Query offsets: %ld -> %ld   %ld -> %ld",
	//	qry->oa[0], qry->ob[0],
	//	qry->oa[1], qry->ob[1] );

	// change this to be a linked list at some point?
	// would that make running functions on them harder?
	qry->points = (PNT *) allocz( qry->count * sizeof( PNT ) );

	// are we looking at the first bucket?  that's simpler, but
	// we ignore the metric, because there is only one option
	if( qry->bkt == 0 )
	{
		file_read_blocks_points( r, b, qry );
	}
	else
	{
		(*(file_reader_functions[qry->metric]))( r, b, qry );
	}
}



