/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem.c - memory control, free list management                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry_test.h"


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



// grab some more memory of the proper size
// must be called inside a lock
void __mtype_alloc_free( MTYPE *mt, int count )
{
	MTBLANK *p, *list;
	void *vp;
	int i;

	if( count <= 0 )
		count = mt->alloc_ct;

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


static inline void *__mtype_new( MTYPE *mt )
{
	MTBLANK *b;

	lock_mem( mt );

	if( !mt->fcount )
		__mtype_alloc_free( mt, 0 );

	b = mt->flist;
	mt->flist = b->next;

	--(mt->fcount);

	unlock_mem( mt );

	b->next = NULL;

	return (void *) b;
}


static inline void *__mtype_new_list( MTYPE *mt, int count )
{
	MTBLANK *top, *end;
	uint32_t c, i;

	if( count <= 0 )
		return NULL;

	c = (uint32_t) count;

	lock_mem( mt );

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

	unlock_mem( mt );

	end->next = NULL;

	return (void *) top;
}



static inline void __mtype_free( MTYPE *mt, void *p )
{
	MTBLANK *b = (MTBLANK *) p;

	lock_mem( mt );

	b->next   = mt->flist;
	mt->flist = p;
	++(mt->fcount);

	unlock_mem( mt );
}


static inline void __mtype_free_list( MTYPE *mt, int count, void *first, void *last )
{
	MTBLANK *l = (MTBLANK *) last;

	lock_mem( mt );

	l->next     = mt->flist;
	mt->flist   = first;
	mt->fcount += count;

	unlock_mem( mt );
}







IOLIST *mem_new_iolist( void )
{
	IOLIST *i;

	i = __mtype_new( ctl->mem->iolist );
	if( !i->inited )
	{
		pthread_mutex_init( &(i->lock), NULL );
		i->inited = 1;
	}

	return i;
}

void mem_free_iolist( IOLIST **l )
{
	IOLIST *sl;

	if( !l || !*l )
		return;

	sl = *l;
	*l = NULL;

	if( sl->head )
		mem_free_buf_list( sl->head );

	sl->head = NULL;
	sl->tail = NULL;
	sl->bufs = 0;

	__mtype_free( ctl->mem->iolist, sl );
}

void mem_free_iolist_list( IOLIST *list )
{
	IOLIST *l, *freed, *end;
	int j = 0;

	freed = end = NULL;

	while( list )
	{
		l    = list;
		list = l->next;

		if( l->head )
			mem_free_buf_list( l->head );

		l->head = NULL;
		l->tail = NULL;
		l->bufs = 0;

		l->next = freed;
		freed   = l;

		if( !end )
			end = l;

		j++;
	}

	__mtype_free_list( ctl->mem->iolist, j, freed, end );
}


IOBUF *mem_new_buf( int sz, int64_t lifetime )
{
	IOBUF *b;

	b = (IOBUF *) __mtype_new( ctl->mem->iobufs );

	if( sz < 0 )
		sz = MIN_NETBUF_SZ;

	if( b->sz < sz )
	{
		if( b->ptr )
			free( b->ptr );

		b->ptr = (char *) allocz( sz );
		b->sz  = sz;
	}

	if( b->sz == 0 )
		b->buf = NULL;
	else
		b->buf = b->ptr;

	b->lifetime = lifetime;
	b->expires  = ctl->proc->curr_tval + b->lifetime;

	return b;
}



void mem_free_buf( IOBUF **b )
{
	IOBUF *sb;

	if( !b || !*b )
		return;

	sb = *b;
	*b = NULL;

	if( sb->sz )
		sb->ptr[0] = '\0';

	sb->prev = NULL;
	sb->buf  = NULL;
	sb->len  = 0;

	__mtype_free( ctl->mem->iobufs, sb );
}


void mem_free_buf_list( IOBUF *list )
{
	IOBUF *b, *freed, *end;
	int j = 0;

	freed = end = NULL;

	while( list )
	{
		b    = list;
		list = b->next;

		if( b->sz )
			b->ptr[0] = '\0';

		b->prev = NULL;
		b->buf  = NULL;
		b->len  = 0;
		b->next = freed;
		freed   = b;

		if( !end )
			end = b;

		j++;
	}

	__mtype_free_list( ctl->mem->iobufs, j, freed, end );
}




void mem_prealloc_one( MTYPE *mt )
{
	double act, fct;
	int add, tot;

	if( !mt->prealloc )
		return;

	act = (double) mt->total;
	fct = (double) mt->fcount;

	add = 2 * mt->alloc_ct;
	tot = 0;

	while( ( fct / act ) < mt->threshold )
	{
		lock_mem( mt );
		__mtype_alloc_free( mt, add );
		unlock_mem( mt );
		tot += add;

		act = (double) mt->total;
		fct = (double) mt->fcount;
	}

	if( tot > 0 )
		debug( "Pre-allocated %d slots for %s", tot, mt->name );
}



void mem_prealloc( int64_t tval, void *arg )
{
	mem_prealloc_one( ctl->mem->iobufs );
	mem_prealloc_one( ctl->mem->iolist );
}


void mem_check( int64_t tval, void *arg )
{
	struct rusage ru;

	getrusage( RUSAGE_SELF, &ru );

	ctl->mem->curr_kb = ru.ru_maxrss;

	if( ctl->mem->curr_kb > ctl->mem->max_kb )
		loop_end( "Memory usage exceeds configured maximum." );
}


void *mem_prealloc_loop( void *arg )
{
	loop_control( "memory pre-alloc", mem_prealloc, NULL, 1000 * ctl->mem->prealloc, LOOP_TRIM, 0 );

	free( (THRD *) arg );
	return NULL;
}

void *mem_check_loop( void *arg )
{
	loop_control( "memory control", mem_check, NULL, 1000 * ctl->mem->interval, LOOP_TRIM, 0 );

	free( (THRD *) arg );
	return NULL;
}



int mem_config_line( AVP *av )
{
	MEM_CTL *m = ctl->mem;
	MTYPE *mt;
	int64_t t;
	char *d;

	if( !( d = strchr( av->att, '.' ) ) )
	{
		// just the singles
		if( attIs( "maxMb" ) || attIs( "maxSize" ) )
		{
			av_int( m->max_kb );
			m->max_kb <<= 10;
		}
		else if( attIs( "maxKb") )
			av_int( m->max_kb );
		else if( attIs( "interval" ) || attIs( "msec" ) )
			av_int( m->interval );
		else if( attIs( "prealloc" ) || attIs( "preallocInterval" ) )
			av_int( m->prealloc );
		else if( attIs( "hashSize" ) )
			av_int( m->hashsize );
		else if( attIs( "stackSize" ) )
		{
			// gets converted to KB
			av_int( m->stacksize );
			debug( "Stack size set to %d KB.", m->stacksize );
		}
		else
			return -1;

		return 0;
	}

	*d++ = '\0';

	// after this, it's per-type control
	if( attIs( "iobufs" ) )
		mt = m->iobufs;
	else if( attIs( "iolist" ) )
		mt = m->iolist;
	else
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
	MEM_CTL *m;

	m = ctl->mem;

	pthread_mutex_destroy( &(m->iobufs->lock) );
	pthread_mutex_destroy( &(m->iolist->lock) );
}


MTYPE *__mem_type_ctl( char *name, int sz, int ct, int extra )
{
	MTYPE *mt;

	mt            = (MTYPE *) allocz( sizeof( MTYPE ) );
	mt->alloc_sz  = sz;
	mt->alloc_ct  = ct;
	mt->stats_sz  = sz + extra;
	mt->name      = str_dup( name, 0 );
	mt->threshold = DEFAULT_MEM_PRE_THRESH;

	// prealloc all types by default
	mt->prealloc = 1;

	// and alloc some already
	__mtype_alloc_free( mt, 4 * mt->alloc_ct );

	// init the mutex
	pthread_mutex_init( &(mt->lock), NULL );

	return mt;
}



MEM_CTL *mem_config_defaults( void )
{
	MEM_CTL *m;

	m = (MEM_CTL *) allocz( sizeof( MEM_CTL ) );

	m->iobufs = __mem_type_ctl( "iobufs", sizeof( IOBUF ),  MEM_ALLOCSZ_IOBUF,  ( MIN_NETBUF_SZ + IO_BUF_SZ ) / 2 );
	m->iolist = __mem_type_ctl( "iolist", sizeof( IOLIST ), MEM_ALLOCSZ_IOLIST, 0 );

	m->max_kb       = DEFAULT_MEM_MAX_KB;
	m->interval     = DEFAULT_MEM_CHECK_INTV;
	m->hashsize     = MEM_HSZ_LARGE;
	m->stacksize    = DEFAULT_MEM_STACK_SIZE;
	m->prealloc     = DEFAULT_MEM_PRE_INTV;

	return m;
}

