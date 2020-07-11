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



int rkv_write_one( SSTE *e, void *arg )
{
	PTL *p, *pts;
	TEL *tel;
	LEAF *l;
	RKFT *f;

	f   = (RKFT *) arg;
	tel = (TEL *) e->ptr;
	l   = tel->leaf;

	if( !l->points )
	{
		if( l->fh
		 && l->last_flushed
		 && ( _proc->curr_time.tv_sec - l->last_flushed ) > _rkv->max_open_sec )
		{
			rkv_close( l->fh );
			l->last_flushed = 0;
		}

		return 0;
	}


	if( !l->fh )
	{
		rkv_tree_lock( tel );

		// TODO: handle persistent failures
		l->fh = rkv_create_handle( tel->path, tel->plen );

		rkv_tree_unlock( tel );

		if( !l->fh )
			return -1;
	}

	if( !l->fh->map )
	{
		// TODO: handle persistent failures
		// push max path length further back? into data.c
		rkv_open( l->fh );

		if( !l->fh->map )
			return -2;
	}

	//info( "Visiting %s.", l->fh->fpath );

	rkv_tree_lock( tel );

	pts = l->points;
	l->points = NULL;

	rkv_tree_unlock( tel );

	// write out the points
	// we don't do this under lock as only
	// this thread every calls
	for( p = pts; p; p = p->next )
	{
		rkv_update( l->fh, p->points, p->count );
		f->up_ops += (int64_t) p->count;
	}

	mem_free_ptlst_list( pts );
	return 0;
}


int rkv_finish_one( SSTE *e, void *arg )
{
	TEL *tel;

	rkv_write_one( e, arg );

	tel = (TEL *) e->ptr;

	if( tel->leaf->fh )
	{
		rkv_shutdown( tel->leaf->fh );
	}

	return 0;
}


void rkv_writer_pass( int64_t tval, void *arg )
{
	double diff;
	int64_t a;
	RKFT *f;

	f = (RKFT *) arg;

	a = get_time64( );
	string_store_iterator_nolock( _rkv->hash, f, &rkv_write_one, f->id, _rkv->wr_threads );

	diff = (double) ( get_time64( ) - a );
	diff /= BILLIONF;

	// keep track of that time
	f->up_time += diff;

	if( diff > 1.0 )
		warn( "[%02lld] Pass took %.6f sec.", f->id, diff );
}



/*
 * Iterate over the hash looking for things to update
 */
void rkv_writer( THRD *t )
{
	// it might seem tempting to tune this by bucket, but
	// this delay is only how long before points become available
	// in searches.  Is that based on buckets, or human patience?
	// Is there a use case for rapid machine querying for non-dashboard purposes?
	loop_control( t->name, &rkv_writer_pass, t->arg, 2000000, LOOP_SYNC, t->num * 250000 );

	notice( "Syncing data to disk after shutdown." );
	string_store_iterator_nolock( _rkv->hash, NULL, &rkv_finish_one, t->num, _rkv->wr_threads );
}



