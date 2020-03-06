/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem/mtype.c - functions for controlled memory types & preallocation     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"





void __mtype_report_counts( MTYPE *mt )
{
#ifdef MTYPE_TRACING
	info( "Mtype %12s:  Flist %p   Fcount %10d    Alloc: calls %12d sum %12d   Free: calls %12d sum %12d",
		mt->name, mt->flist, mt->ctrs.fcount,
		mt->ctrs.all.ctr, mt->ctrs.all.sum,
		mt->ctrs.fre.ctr, mt->ctrs.fre.sum );
#endif
}


// grab some more memory of the proper size
// must be called inside a lock
void __mtype_alloc_free( MTYPE *mt, int count, int prefetch )
{
	MTBLANK *p, *list;
	void *vp;
	int i;

	if( count <= 0 )
		count = mt->alloc_ct;

	if( !mt->flist )
	{
		if( mt->ctrs.fcount > 0 )
		{
			err( "Mtype %s flist null but fcount %d.", mt->name, mt->ctrs.fcount );
			__mtype_report_counts( mt );
			//mt->ctrs.fcount = 0;
		}
	}
	else if( !mt->ctrs.fcount )
	{
		err( "Mtype %s flist set but fcount 0.", mt->name );
		__mtype_report_counts( mt );
		//for( i = 0, p = mt->flist; p; p = p->next; ++i );
		//mt->ctrs.fcount = i;
	}

	list = (MTBLANK *) allocz( mt->alloc_sz * count );

	if( !list )
		fatal( "Failed to allocate %d * %d bytes.", mt->alloc_sz, count );

	mt->ctrs.fcount += count;
	mt->ctrs.total  += count;

#ifdef MTYPE_TRACING
	if( prefetch )
	{
		++(mt->ctrs.pre.ctr);
		mt->ctrs.pre.sum += count;
	}
	else
	{
		++(mt->ctrs.ref.ctr);
		mt->ctrs.ref.sum += count;
	}
#endif

	// the last one needs next -> flist so decrement count
	count--;

	vp = list;
	p  = list;

	// link them up
	for( i = 0; i < count; ++i )
	{
#ifdef __GNUC__
#if __GNUC_PREREQ(4,8)
#pragma GCC diagnostic ignored "-Wpointer-arith"
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#endif
		// yes GCC, I know what I'm doing, thanks
		//
		// We allocate memory of sz * count
		// Then step through it by sz, allocating
		// the next pointers of the generic MTBLANK
		// list.  When we are asked for a struct off
		// this list, we explicitly cast it, so the
		// next pointer of the desired struct points
		// upwards by the size of the desired struct
		// 
		// But GCC freaks out because it's noticed
		// we are adding more than the size of the
		// struct size to vp, ie, not doing vp++
		// and wonders if it's a bug.
		vp     += mt->alloc_sz;
#ifdef __GNUC__
#if __GNUC_PREREQ(4,8)
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif
#endif
		p->next = (MTBLANK *) vp;
		p       = p->next;
	}

	// and attach to the free list (it might not be null)
	p->next = mt->flist;

	// and update our type
	mt->flist = list;
}


inline void *mtype_new( MTYPE *mt )
{
	MTBLANK *b;

	mem_lock( mt );

	if( !mt->ctrs.fcount || !mt->flist )
		__mtype_alloc_free( mt, 0, 0 );

	b = mt->flist;
	mt->flist = b->next;

	--(mt->ctrs.fcount);

#ifdef MTYPE_TRACING
	++(mt->ctrs.all.ctr);
	++(mt->ctrs.all.sum);
#endif

	mem_unlock( mt );

	b->next = NULL;

	return (void *) b;
}


inline void *mtype_new_list( MTYPE *mt, int count )
{
	MTBLANK *top, *end;
	uint32_t c, i;

	if( count <= 0 )
		return NULL;

	c = (uint32_t) count;

	mem_lock( mt );

	// get enough
	while( mt->ctrs.fcount < c )
		__mtype_alloc_free( mt, 0, 0 );

	top = end = mt->flist;

	// run down count - 1 elements
	for( i = c - 1; i > 0; i-- )
		end = end->next;

	// end is now the last in the list we want
	mt->flist = end->next;
	mt->ctrs.fcount -= c;

#ifdef MTYPE_TRACING
	++(mt->ctrs.all.ctr);
	mt->ctrs.all.sum += c;
#endif

	mem_unlock( mt );

	end->next = NULL;

	return (void *) top;
}



inline void mtype_free( MTYPE *mt, void *p )
{
	MTBLANK *b = (MTBLANK *) p;

	mem_lock( mt );

	b->next   = mt->flist;
	mt->flist = p;
	++(mt->ctrs.fcount);

#ifdef MTYPE_TRACING
	++(mt->ctrs.fre.ctr);
	++(mt->ctrs.fre.sum);
#endif

	mem_unlock( mt );
}


inline void mtype_free_list( MTYPE *mt, int count, void *first, void *last )
{
	MTBLANK *l = (MTBLANK *) last;

	mem_lock( mt );

	l->next   = mt->flist;
	mt->flist = first;
	mt->ctrs.fcount += count;

#ifdef MTYPE_TRACING
	++(mt->ctrs.fre.ctr);
	mt->ctrs.fre.sum += count;
#endif

	mem_unlock( mt );
}


void mem_prealloc_one( MTYPE *mt )
{
	double act, fct;
	int add, tot;

	if( !mt->prealloc )
		return;

	act = (double) mt->alloc_ct;
	fct = (double) mt->ctrs.fcount;

	add = 2 * mt->alloc_ct;
	tot = 0;

	//debug( "Mem type %s free/alloc ratio is %.2f, threshold %.2f", mt->name, fct / act, mt->threshold );

	while( ( fct / act ) < mt->threshold )
	{
		mem_lock( mt );
		__mtype_alloc_free( mt, add, 1 );
		mem_unlock( mt );
		tot += add;

		act = (double) mt->ctrs.total;
		fct = (double) mt->ctrs.fcount;
	}

	if( tot > 0 )
		debug( "Pre-allocated %d slots for %s", tot, mt->name );
}



void mem_prealloc( int64_t tval, void *arg )
{
	MTYPE *mt;
	int i;

	for( i = 0; i < MEM_TYPES_MAX; ++i )
	{
		if( !( mt = _mem->types[i] ) )
			break;

		mem_prealloc_one( mt );
	}
}


void mem_prealloc_loop( THRD *t )
{
	loop_control( "memory pre-alloc", mem_prealloc, NULL, 1000 * _mem->prealloc, LOOP_TRIM, 0 );
}



MTYPE *mem_type_declare( char *name, int sz, int ct, int extra, uint32_t pre )
{
	MTYPE *mt;

	mt            = (MTYPE *) allocz( sizeof( MTYPE ) );
	mt->alloc_sz  = sz;
	mt->alloc_ct  = ct;
	mt->stats_sz  = sz + extra;
	mt->name      = str_perm( name, 0 );
	mt->threshold = DEFAULT_MEM_PRE_THRESH;
	mt->prealloc  = pre;

	// get our ID, and place us in the list
	mt->id        = _mem->type_ct++;

	_mem->types[mt->id] = mt;

	// and alloc some already
	__mtype_alloc_free( mt, 4 * mt->alloc_ct, 1 );

	// init the mutex
	pthread_mutex_init( &(mt->lock), &(_mem->mtxa) );

	return mt;
}


int mem_type_stats( int id, MTSTAT *ms )
{
	MTYPE *m = _mem->types[id];

	memset( ms, 0, sizeof( MTSTAT ) );

	if( id >= _mem->type_ct )
		return -1;

	// grab the stats
	mem_lock( m );
	memcpy( &(ms->ctrs), &(m->ctrs), sizeof( MTCTR ) );
	mem_unlock( m );

	// these can be after the unlock
	ms->bytes = (uint64_t) m->stats_sz * (uint64_t) ms->ctrs.total;
	ms->name = m->name;

	return 0;
}



