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
* rkv/read.c - read points from file                                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

#define rkrb_loop0( _p )		for( i = q->oa[0]; i < q->ob[0]; ++i, ++j, ++_p )
#define rkrb_loop1( _p )		for( i = 0; i < q->ob[1]; ++i, ++j, ++_p )

#define rkrb_setpt( _t, _d )	if( _t >= q->first && _t <= q->last ) { q->data->points[j].ts = _t; q->data->points[j].val = _d; ++(q->pcount); }

#define rkrb_setupP( )			j = 0; parr = (PNT *) q->fl->ptrs[0]; p = parr + q->oa[0]
#define rkrb_setupA( )			j = 0; aarr = (PNTA *) q->fl->ptrs[q->bkt]; a = aarr + q->oa[0]

#define rkrb_check1P( )			if( q->ob[1] == 0 ) { return; } p = parr
#define rkrb_check1A( )			if( q->ob[1] == 0 ) { return; } a = aarr


void rkv_read_blocks_points( RKQR *q )
{
	PNT *parr, *p;
	int64_t i, j;

	rkrb_setupP( );

	rkrb_loop0( p )
	{
		rkrb_setpt( p->ts, p->val );
	}

	rkrb_check1P( );

	rkrb_loop1( p )
	{
		rkrb_setpt( p->ts, p->val );
	}
}

void rkv_read_blocks_mean( RKQR *q )
{
	PNTA *aarr, *a;
	int64_t i, j;

	rkrb_setupA( );

	rkrb_loop0( a )
	{
//		if( a->ts > 0 )
//		{
//			debug( "Point-array pointer %p, %lld / %f - %d", a, a->ts, a->sum, a->count );
//			debug( "Timestamp match:  %lld <= %lld <= %lld  ??  %d", q->first, a->ts, q->last,
//				( a->ts >= q->first && a->ts <= q->last ) );
//		}
		rkrb_setpt( a->ts, ( a->sum / (double) a->count ) );
	}

	rkrb_check1A( );

	rkrb_loop1( a )
	{
		rkrb_setpt( a->ts, ( a->sum / (double) a->count ) );
	}
}

void rkv_read_blocks_count( RKQR *q )
{
	PNTA *aarr, *a;
	int64_t i, j;

	rkrb_setupA( );

	rkrb_loop0( a )
	{
		rkrb_setpt( a->ts, ( (double) a->count ) );
	}

	rkrb_check1A( );

	rkrb_loop1( a )
	{
		rkrb_setpt( a->ts, ( (double) a->count ) );
	}
}


void rkv_read_blocks_sum( RKQR *q )
{
	PNTA *aarr, *a;
	int64_t i, j;

	rkrb_setupA( );

	rkrb_loop0( a )
	{
		rkrb_setpt( a->ts, a->sum );
	}

	rkrb_check1A( );

	rkrb_loop1( a )
	{
		rkrb_setpt( a->ts, a->sum );
	}
}


void rkv_read_blocks_min( RKQR *q )
{
	PNTA *aarr, *a;
	int64_t i, j;

	rkrb_setupA( );

	rkrb_loop0( a )
	{
		rkrb_setpt( a->ts, a->min );
	}

	rkrb_check1A( );

	rkrb_loop1( a )
	{
		rkrb_setpt( a->ts, a->min );
	}
}


void rkv_read_blocks_max( RKQR *q )
{
	PNTA *aarr, *a;
	int64_t i, j;

	rkrb_setupA( );

	rkrb_loop0( a )
	{
		rkrb_setpt( a->ts, a->max );
	}

	rkrb_check1A( );

	rkrb_loop1( a )
	{
		rkrb_setpt( a->ts, a->max );
	}
}


void rkv_read_blocks_spread( RKQR *q )
{
	PNTA *aarr, *a;
	int64_t i, j;

	rkrb_setupA( );

	rkrb_loop0( a )
	{
		rkrb_setpt( a->ts, ( a->max - a->min ) );
	}

	rkrb_check1A( );

	rkrb_loop1( a )
	{
		rkrb_setpt( a->ts, ( a->max - a->min ) );
	}
}


void rkv_read_blocks_middle( RKQR *q )
{
	PNTA *aarr, *a;
	int64_t i, j;

	rkrb_setupA( );

	rkrb_loop0( a )
	{
		rkrb_setpt( a->ts, ( ( a->max + a->min ) / 2.0 ) );
	}

	rkrb_check1A( );

	rkrb_loop1( a )
	{
		rkrb_setpt( a->ts, ( ( a->max + a->min ) / 2.0 ) );
	}
}


// proportion of spread against the mean
void rkv_read_blocks_range( RKQR *q )
{
	PNTA *aarr, *a;
	int64_t i, j;

	rkrb_setupA( );

	rkrb_loop0( a )
	{
		if( a->count && a->sum != 0.0 )
		{
			rkrb_setpt( a->ts, ( ( a->max - a->min ) / ( a->sum / (double) a->count ) ) );
		}
	}

	rkrb_check1A( );

	rkrb_loop1( a )
	{
		if( a->count && a->sum != 0.0 )
		{
			rkrb_setpt( a->ts, ( ( a->max - a->min ) / ( a->sum / (double) a->count ) ) );
		}
	}
}


rkv_rd_fn *rkv_reader_functions[RKV_VAL_METR_END] =
{
	&rkv_read_blocks_mean,
	&rkv_read_blocks_count,
	&rkv_read_blocks_sum,
	&rkv_read_blocks_min,
	&rkv_read_blocks_max,
	&rkv_read_blocks_spread,
	&rkv_read_blocks_middle,
	&rkv_read_blocks_range
};

const char *rkv_metric_names[RKV_VAL_METR_END] =
{
	"mean",
	"count",
	"sum",
	"min",
	"max",
	"spread",
	"middle",
	"range"
};


int rkv_choose_metric( const char *name )
{
	int i;

	if( !name || !*name )
		return RKV_VAL_METR_MEAN;

	i = str_search( name, rkv_metric_names, RKV_VAL_METR_END );

	if( i < 0 )
	{
		warn( "File metric name '%s' not recognised.", name );
		i = RKV_VAL_METR_MEAN;
	}

	return i;
}

const char *rkv_report_metric( int mval )
{
	if( mval < 0 || mval >= RKV_VAL_METR_END )
		return "invalid";

	return rkv_metric_names[mval];
}



int rkv_choose_bucket( RKFL *r, int64_t from, int64_t to )
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
void rkv_read( RKFL *r, RKQR *qry )
{
	int64_t x, y, count;

	if( !r->map && rkv_open( r ) != 0 )
	{
		warn( "Reading from unopened and unopenable rkv failed." );
		return;
	}

	qry->fl  = r;
	qry->bkt = rkv_choose_bucket( r, qry->from, qry->to );
	qry->b   = r->hdr->buckets + qry->bkt;

	// correct the timestamps to be bucket-aligned
	qry->first = qry->from - ( qry->from % qry->b->period );
	qry->last  = qry->to   - ( qry->to   % qry->b->period );

	x = ( qry->first / qry->b->period ) % qry->b->count;
	y = ( qry->last  / qry->b->period ) % qry->b->count;

	// is it in one block or two
	if( x < y )
	{
		count = (int) ( y - x );
		qry->oa[0] = x;
		qry->ob[0] = y;
	}
	else
	{
		count = (int) y + (int) ( qry->b->count - x );
		qry->oa[0] = x;
		qry->ob[0] = qry->b->count;
		qry->oa[1] = 0;
		qry->ob[1] = y;
	}

//	debug( "Query offsets: [%d/%lld] from   %ld -> %ld   %ld -> %ld",
//		qry->bkt, count,
//		qry->oa[0], qry->ob[0],
//		qry->oa[1], qry->ob[1] );

	qry->data = mem_new_ptser( count );

	// we need this for PTL functions
	qry->data->period = qry->b->period;

	// are we looking at the first bucket?  that's simpler, but
	// we ignore the metric, because there is only one option
	if( qry->bkt == 0 )
	{
		rkv_read_blocks_points( qry );
	}
	else
	{
		(*(rkv_reader_functions[qry->metric]))( qry );
	}
}



