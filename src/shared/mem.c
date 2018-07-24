/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem.c - memory control, free list management                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"


uint32_t mem_alloc_size( int len )
{
	uint32_t l, v = 1;
	int k, h = 0;

	l = (uint32_t) len;

	// find the highest bit set
	for( h = 0, k = 0; k < 32; k++ )
	{
		if( l & v )
			h = k;

		v <<= 1;
	}

	if( h > 15 )
		return len + 1;

	return 2 << h;
}


void *mem_reverse_list( void *list_in )
{
	MTBLANK *one, *list, *ret = NULL;

	list = (MTBLANK *) list_in;

	while( list )
	{
		one  = (MTBLANK *) list;
		list = (MTBLANK *) one->next;

		one->next = (void *) ret;
		ret = (MTBLANK *) one;
	}

	return (void *) ret;
}


void __mtype_report_counts( MTYPE *mt )
{
#ifdef MTYPE_TRACING
	info( "Mtype %12s:  Flist %p   Fcount %10d    Alloc: calls %12d sum %12d   Free: calls %12d sum %12d",
		mt->name, mt->flist, mt->fcount,
		mt->a_call_ctr, mt->a_call_sum,
		mt->f_call_ctr, mt->f_call_sum );
#endif
}


// grab some more memory of the proper size
// must be called inside a lock
void __mtype_alloc_free( MTYPE *mt, int count )
{
	MTBLANK *p, *list;
	void *vp;
	int i;

	if( count <= 0 )
		count = mt->alloc_ct;

	if( !mt->flist )
	{
		if( mt->fcount > 0 )
		{
			err( "Mtype %s flist null but fcount %d.", mt->name, mt->fcount );
			__mtype_report_counts( mt );
			//mt->fcount = 0;
		}
	}
	else if( !mt->fcount )
	{
		err( "Mtype %s flist set but fcount 0.", mt->name );
		__mtype_report_counts( mt );
		//for( i = 0, p = mt->flist; p; p = p->next; i++ );
		//mt->fcount = i;
	}

	list = (MTBLANK *) allocz( mt->alloc_sz * count );

	if( !list )
		fatal( "Failed to allocate %d * %d bytes.", mt->alloc_sz, count );

	mt->fcount += count;
	mt->total  += count;

	// the last one needs next -> flist so decrement count
	count--;

	vp = list;
	p  = list;

	// link them up
	for( i = 0; i < count; i++ )
	{
#ifdef __GNUC__
#if __GNUC_PREREQ(4,8)
#pragma GCC diagnostic ignored "-Wpointer-arith"
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#endif
		// yes GCC, I know what I'm doing, thanks
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

	if( !mt->fcount || !mt->flist )
		__mtype_alloc_free( mt, 0 );

	b = mt->flist;
	mt->flist = b->next;

	--(mt->fcount);

#ifdef MTYPE_TRACING
	mt->a_call_ctr++;
	mt->a_call_sum++;
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
	while( mt->fcount < c )
		__mtype_alloc_free( mt, 0 );

	top = end = mt->flist;

	// run down count - 1 elements
	for( i = c - 1; i > 0; i-- )
		end = end->next;

	// end is now the last in the list we want
	mt->flist   = end->next;
	mt->fcount -= c;

#ifdef MTYPE_TRACING
	mt->a_call_ctr++;
	mt->a_call_sum += c;
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
	++(mt->fcount);

#ifdef MTYPE_TRACING
	mt->f_call_ctr++;
	mt->f_call_sum++;
#endif

	mem_unlock( mt );
}


inline void mtype_free_list( MTYPE *mt, int count, void *first, void *last )
{
	MTBLANK *l = (MTBLANK *) last;

	mem_lock( mt );

	l->next     = mt->flist;
	mt->flist   = first;
	mt->fcount += count;

#ifdef MTYPE_TRACING
	mt->f_call_ctr++;
	mt->f_call_sum += count;
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
	fct = (double) mt->fcount;

	add = 2 * mt->alloc_ct;
	tot = 0;

	//debug( "Mem type %s free/alloc ratio is %.2f, threshold %.2f", mt->name, fct / act, mt->threshold );

	while( ( fct / act ) < mt->threshold )
	{
		mem_lock( mt );
		__mtype_alloc_free( mt, add );
		mem_unlock( mt );
		tot += add;

		act = (double) mt->total;
		fct = (double) mt->fcount;
	}

	if( tot > 0 )
		debug( "Pre-allocated %d slots for %s", tot, mt->name );
}



void mem_prealloc( int64_t tval, void *arg )
{
	MTYPE *mt;
	int i;

	for( i = 0; i < MEM_TYPES_MAX; i++ )
	{
		if( !( mt = _mem->types[i] ) )
			break;

		mem_prealloc_one( mt );
	}
}


void mem_check( int64_t tval, void *arg )
{
	struct rusage ru;

	getrusage( RUSAGE_SELF, &ru );

	_mem->curr_kb = ru.ru_maxrss;

	if( _mem->curr_kb > _mem->max_kb )
		loop_end( "Memory usage exceeds configured maximum." );
}


void *mem_prealloc_loop( void *arg )
{
	loop_control( "memory pre-alloc", mem_prealloc, NULL, 1000 * _mem->prealloc, LOOP_TRIM, 0 );

	free( (THRD *) arg );
	return NULL;
}

void *mem_check_loop( void *arg )
{
	// do it now - for stats
	mem_check( 0, NULL );

	loop_control( "memory control", mem_check, NULL, 1000 * _mem->interval, LOOP_TRIM, 0 );

	free( (THRD *) arg );
	return NULL;
}



int mem_config_line( AVP *av )
{
	MTYPE *mt;
	int64_t t;
	char *d;
	int i;

	if( !( d = strchr( av->att, '.' ) ) )
	{
		// just the singles
		if( attIs( "maxMb" ) || attIs( "maxSize" ) )
		{
			av_int( _mem->max_kb );
			_mem->max_kb <<= 10;
		}
		else if( attIs( "maxKb") )
			av_int( _mem->max_kb );
		else if( attIs( "interval" ) || attIs( "msec" ) )
			av_int( _mem->interval );
		else if( attIs( "prealloc" ) || attIs( "preallocInterval" ) )
			av_int( _mem->prealloc );
		else if( attIs( "doChecks" ) )
			_mem->do_checks = config_bool( av );
		else if( attIs( "stackSize" ) )
		{
			// gets converted to KB
			av_int( _mem->stacksize );
			debug( "Stack size set to %d KB.", _mem->stacksize );
		}
		else if( attIs( "stackHighSize" ) )
		{
			// gets converted to KB
			av_int( _mem->stackhigh );
			debug( "Stack high size set to %d KB.", _mem->stackhigh );
		}
		else
			return -1;

		return 0;
	}

	*d++ = '\0';

	// after this, it's per-type control
	// so go find it.
	for( i = 0; i < MEM_TYPES_MAX; i++ )
	{
		if( !( mt = _mem->types[i] ) )
			break;

		if( attIs( mt->name ) )
			break;
	}

	if( !mt )
		return -1;

	if( !strcasecmp( d, "block" ) )
	{
		av_int( t );
		if( t > 0 )
		{
			mt->alloc_ct = (uint32_t) t;
			debug( "Allocation block for %s set to %u", mt->name, mt->alloc_ct );
		}
	}
	else if( !strcasecmp( d, "prealloc" ) )
	{
		mt->prealloc = 0;
		debug( "Preallocation disabled for %s", mt->name );
	}
	else if( !strcasecmp( d, "threshold" ) )
	{
		av_dbl( mt->threshold );
		if( mt->threshold < 0 || mt->threshold >= 1 )
		{
			warn( "Invalid memory type threshold %f for %s - resetting to default.", mt->threshold, mt->name );
			mt->threshold = DEFAULT_MEM_PRE_THRESH;
		}
		else if( mt->threshold > 0.5 )
			warn( "Memory type threshold for %s is %f - that's quite high!", mt->name, mt->threshold );

		debug( "Mem prealloc threshold for %s is now %f", mt->name, mt->threshold );
	}
	else
		return -1;

	// done this way because GC might become a thing

	return 0;
}

// shut down memory locks
void mem_shutdown( void )
{
	MTYPE *mt;
	int i;

	for( i = 0; i < MEM_TYPES_MAX; i++ )
	{
		if( !( mt = _mem->types[i] ) )
			break;

		pthread_mutex_destroy( &(mt->lock) );
	}
}

// do we do checks?
void mem_startup( void )
{
	if( _mem->do_checks )
		thread_throw( &mem_check_loop, NULL, 0 );

	thread_throw( &mem_prealloc_loop, NULL, 0 );
}



MTYPE *mem_type_declare( char *name, int sz, int ct, int extra, uint32_t pre )
{
	MTYPE *mt;

	mt            = (MTYPE *) allocz( sizeof( MTYPE ) );
	mt->alloc_sz  = sz;
	mt->alloc_ct  = ct;
	mt->stats_sz  = sz + extra;
	mt->name      = str_dup( name, 0 );
	mt->threshold = DEFAULT_MEM_PRE_THRESH;
	mt->prealloc  = pre;

	// get our ID, and place us in the list
	mt->id        = _mem->type_ct++;

	_mem->types[mt->id] = mt;

	// and alloc some already
	__mtype_alloc_free( mt, 4 * mt->alloc_ct );

	// init the mutex
	pthread_mutex_init( &(mt->lock), &(_proc->mtxa) );

	return mt;
}


int mem_type_stats( int id, MTSTAT *ms )
{
	MTYPE *m = _mem->types[id];

	memset( ms, 0, sizeof( MTSTAT ) );

	if( id >= _mem->type_ct )
		return -1;

	mem_lock( m );

	ms->freec = m->fcount;
	ms->alloc = m->total;
	ms->bytes = (uint64_t) m->stats_sz * (uint64_t) m->total;

	mem_unlock( m );

	ms->name = m->name;

	return 0;
}

int64_t mem_curr_kb( void )
{
	return _mem->curr_kb;
}



MEM_CTL *mem_config_defaults( void )
{
	_mem = (MEM_CTL *) allocz( sizeof( MEM_CTL ) );

	_mem->max_kb       = DEFAULT_MEM_MAX_KB;
	_mem->interval     = DEFAULT_MEM_CHECK_INTV;
	_mem->stacksize    = DEFAULT_MEM_STACK_SIZE;
	_mem->stackhigh    = DEFAULT_MEM_STACK_HIGH;
	_mem->prealloc     = DEFAULT_MEM_PRE_INTV;

	_mem->iobufs       = mem_type_declare( "iobufs", sizeof( IOBUF ), MEM_ALLOCSZ_IOBUF, IO_BUF_SZ, 1 );
	_mem->iobps        = mem_type_declare( "iobps",  sizeof( IOBP ),  MEM_ALLOCSZ_IOBP,  0, 1 );

	return _mem;
}



// types

IOBUF *mem_new_iobuf( int sz )
{
	IOBUF *b;

	b = (IOBUF *) mtype_new( _mem->iobufs );

	if( sz < 0 )
		sz = IO_BUF_SZ;

	if( b->sz < sz )
	{
		if( b->ptr )
			free( b->ptr );

		b->ptr = (char *) allocz( sz );
		b->sz  = sz;
	}

	if( b->sz == 0 )
	{
		b->buf  = NULL;
		b->hwmk = 0;
	}
	else
	{
		b->buf  = b->ptr;
		b->hwmk = ( 5 * b->sz ) / 6;
	}

	b->refs = 0;

	return b;
}



void mem_free_iobuf( IOBUF **b )
{
	IOBUF *sb;

	if( !b || !*b )
		return;

	sb = *b;
	*b = NULL;

	if( sb->sz )
		sb->ptr[0] = '\0';

	sb->buf  = NULL;
	sb->len  = 0;
	sb->refs = 0;

	sb->mtime    = 0;
	sb->expires  = 0;
	sb->lifetime = 0;

	sb->flags    = 0;

	mtype_free( _mem->iobufs, sb );
}


void mem_free_iobuf_list( IOBUF *list )
{
	IOBUF *b, *freed, *end;
	int j = 0;

	freed = NULL;
	end   = list;

	while( list )
	{
		b    = list;
		list = b->next;

		if( b->sz )
			b->ptr[0] = '\0';

		b->buf  = NULL;
		b->len  = 0;
		b->refs = 0;

		b->mtime    = 0;
		b->expires  = 0;
		b->lifetime = 0;

		b->flags = 0;

		b->next = freed;
		freed   = b;

		j++;
	}

	mtype_free_list( _mem->iobufs, j, freed, end );
}


IOBP *mem_new_iobp( void )
{
	return (IOBP *) mtype_new( _mem->iobps );
}

void mem_free_iobp( IOBP **b )
{
	IOBP *p;

	p  = *b;
	*b = NULL;

	p->buf  = NULL;
	p->prev = NULL;

	mtype_free( _mem->iobps, p );
}

