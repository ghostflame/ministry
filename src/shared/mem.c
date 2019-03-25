/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem.c - memory control, free list management                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"

MEM_CTL *_mem = NULL;

static uint8_t mem_alloc_size_vals[16] =
{
	0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4
};

uint32_t mem_alloc_size( int len )
{
	uint32_t l = (uint32_t) len;
	uint8_t s = 0;

	if( l & 0xffff0000 )
		return len + 1;

	if( l & 0xff00 )
	{
		s += 8;
		l >>= 8;
	}
	if( l & 0xf0 )
	{
		s += 4;
		l >>= 4;
	}

	s += mem_alloc_size_vals[l];

	// minimum 16 bytes
	return 1 << ( ( s > 4 ) ? s : 4 );
}


/*
 * This is a chunk of memory we hand back if calloc failed.
 *
 * The idea is that we hand back this highly distinctive chunk
 * of space.  Any pointer using it will fail.  Any attempt to
 * write to it should fail.
 *
 * So it should give us a core dump at once, right at the
 * problem.  Of course, threads will "help"...
 */

#ifdef USE_MEM_SIGNAL_ARRAY
static const uint8_t mem_signal_array[128] = {
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0
};
#endif


// zero'd memory
void *allocz( size_t size )
{
	void *p = calloc( 1, size );

#ifdef USE_MEM_SIGNAL_ARRAY
	if( !p )
		p = &mem_signal_array;
#endif

	return p;
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

void mem_sort_list( void **list, int count, sort_fn *cmp )
{
	MTBLANK **tmp, *m;
	int i, j;

	if( !list || !*list )
		return;

	if( count == 1 )
		return;

	tmp = (MTBLANK **) allocz( count * sizeof( MTBLANK * ) );
	for( i = 0, m = (MTBLANK *) *list; m; m = m->next )
		tmp[i++] = m;

	qsort( tmp, count, sizeof( MTBLANK * ), cmp );

	// relink them
	j = count - 1;
	for( i = 0; i < j; i++ )
		tmp[i]->next = tmp[i+1];

	tmp[j]->next = NULL;
	*list = tmp[0];

	free( tmp );
}


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
		//for( i = 0, p = mt->flist; p; p = p->next; i++ );
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
		mt->ctrs.pre.ctr++;
		mt->ctrs.pre.sum += count;
	}
	else
	{
		mt->ctrs.ref.ctr++;
		mt->ctrs.ref.sum += count;
	}
#endif

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

	if( !mt->ctrs.fcount || !mt->flist )
		__mtype_alloc_free( mt, 0, 0 );

	b = mt->flist;
	mt->flist = b->next;

	--(mt->ctrs.fcount);

#ifdef MTYPE_TRACING
	mt->ctrs.all.ctr++;
	mt->ctrs.all.sum++;
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
	mt->ctrs.all.ctr++;
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
	mt->ctrs.fre.ctr++;
	mt->ctrs.fre.sum++;
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
	mt->ctrs.fre.ctr++;
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

	for( i = 0; i < MEM_TYPES_MAX; i++ )
	{
		if( !( mt = _mem->types[i] ) )
			break;

		mem_prealloc_one( mt );
	}
}


void mem_check( int64_t tval, void *arg )
{
	MCHK *m = _mem->mcheck;
	int sz = m->bsize;
	struct rusage ru;


	// OK, this appears to be total crap sometimes
	getrusage( RUSAGE_SELF, &ru );
	//info( "getrusage reports %d KB", ru.ru_maxrss );
	m->rusage_kb = ru.ru_maxrss;

	// as does this
	// lets try reading /proc
	if( read_file( "/proc/self/stat", &(m->buf), &sz, 0, "memory file" ) != 0 )
	{
		err( "Cannot read /proc/self/stat, so cannot assess memory." );
		return;
	}
	if( strwords( m->w, m->buf, sz, ' ' ) < 24 )
	{
		err( "File /proc/self/stat not in expected format." );
		return;
	}

	m->proc_kb = atoi( m->w->wd[23] ) * m->psize;
	//info( "/proc/self/stat[23] reports %d KB", m->proc_kb );

	// get our virtual size - comes in bytes, turn into kb
	m->virt_kb = atoi( m->w->wd[22] ) >> 10;

	// use the higher of the two
	m->curr_kb = ( m->rusage_kb > m->proc_kb ) ? m->rusage_kb : m->proc_kb;

	if( m->curr_kb > m->max_kb )
	{
		warn( "Memory usage (%ld KB) exceeds configured maximum (%ld KB), exiting.", m->curr_kb, m->max_kb );
		loop_end( "Memory usage exceeds configured maximum." );
	}
}


void mem_prealloc_loop( THRD *t )
{
	loop_control( "memory pre-alloc", mem_prealloc, NULL, 1000 * _mem->prealloc, LOOP_TRIM, 0 );
}

void mem_check_loop( THRD *t )
{
	MCHK *m = _mem->mcheck;

	m->bsize = 2048;
	m->buf   = (char *) allocz( m->bsize );
	m->w     = (WORDS *) allocz( sizeof( WORDS ) );

	// do it now - for stats
	mem_check( 0, NULL );

	loop_control( "memory control", mem_check, NULL, 1000 * m->interval, LOOP_TRIM, 0 );
}



int mem_config_line( AVP *av )
{
	MTYPE *mt;
	int64_t t;
	char *d;
	int i;

	if( !( d = strchr( av->aptr, '.' ) ) )
	{
		// just the singles
		if( attIs( "maxMb" ) || attIs( "maxSize" ) )
		{
			av_int( _mem->mcheck->max_kb );
			_mem->mcheck->max_kb <<= 10;
		}
		else if( attIs( "maxKb") )
			av_int( _mem->mcheck->max_kb );
		else if( attIs( "interval" ) || attIs( "msec" ) )
			av_int( _mem->mcheck->interval );
		else if( attIs( "doChecks" ) )
			_mem->mcheck->checks = config_bool( av );
		else if( attIs( "prealloc" ) || attIs( "preallocInterval" ) )
			av_int( _mem->prealloc );
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

	av->alen -= d - av->aptr;
	av->aptr  = d;

	if( attIs( "block" ) )
	{
		av_int( t );
		if( t > 0 )
		{
			mt->alloc_ct = (uint32_t) t;
			debug( "Allocation block for %s set to %u", mt->name, mt->alloc_ct );
		}
	}
	else if( attIs( "prealloc" ) )
	{
		mt->prealloc = 0;
		debug( "Preallocation disabled for %s", mt->name );
	}
	else if( attIs( "threshold" ) )
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
	if( _mem->mcheck->checks )
		thread_throw_named( &mem_check_loop, NULL, 0, "mem_check_loop" );

	thread_throw_named( &mem_prealloc_loop, NULL, 0, "mem_prealloc" );
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
	__mtype_alloc_free( mt, 4 * mt->alloc_ct, 1 );

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

	// grab the stats
	mem_lock( m );
	memcpy( &(ms->ctrs), &(m->ctrs), sizeof( MTCTR ) );
	mem_unlock( m );

	// these can be after the unlock
	ms->bytes = (uint64_t) m->stats_sz * (uint64_t) ms->ctrs.total;
	ms->name = m->name;

	return 0;
}


int64_t mem_curr_kb( void )
{
	//info( "_mem->mcheck->curr_kb reports %d KB.", _mem->mcheck->curr_kb );
	return _mem->mcheck->curr_kb;
}

int64_t mem_virt_kb( void )
{
	return _mem->mcheck->virt_kb;
}


MEM_CTL *mem_config_defaults( void )
{
	MCHK *mc = (MCHK *) allocz( sizeof( MCHK ) );

	mc->max_kb        = DEFAULT_MEM_MAX_KB;
	mc->interval      = DEFAULT_MEM_CHECK_INTV;
	mc->psize         = getpagesize( ) >> 10;
	mc->checks        = 0;

	_mem              = (MEM_CTL *) allocz( sizeof( MEM_CTL ) );
	_mem->mcheck      = mc;
	_mem->prealloc    = DEFAULT_MEM_PRE_INTV;

	_mem->iobufs      = mem_type_declare( "iobufs", sizeof( IOBUF ), MEM_ALLOCSZ_IOBUF, IO_BUF_SZ, 1 );
	_mem->iobps       = mem_type_declare( "iobps",  sizeof( IOBP ),  MEM_ALLOCSZ_IOBP,  0, 1 );
	_mem->htreq       = mem_type_declare( "htreqs", sizeof( HTREQ ), MEM_ALLOCSZ_HTREQ, 2048, 0 );

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


HTREQ *mem_new_request( void )
{
	return (HTREQ *) mtype_new( _mem->htreq );
}


void mem_free_request( HTREQ **h )
{
	HTTP_POST *p = NULL;
	BUF *b = NULL;
	HTREQ *r;

	r  = *h;
	*h = NULL;

	b = r->text;
	p = r->post;

	memset( r, 0, sizeof( HTREQ ) );

	if( p )
		memset( p, 0, sizeof( HTTP_POST ) );

	if( b )
	{
		b->buf[0] = '\0';
		b->len = 0;
	}

	r->text = b;
	r->post = p;

	mtype_free( _mem->htreq, r );
}

