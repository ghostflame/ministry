/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem.c - memory control, free list management                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"




// grab some more memory of the proper size
// must be called inside a lock
void __mtype_alloc_free( MTYPE *mt, int count )
{
	MTBLANK *p, *list;
	int i, max;
	void *vp;

	if( !count )
		count = mt->alloc_ct;

	list = (MTBLANK *) allocz( mt->alloc_sz * count );

	// the last one needs a null next
	max = count - 1;

	vp = p = list;

	// link them up
	for( i = 0; i < max; i++ )
	{
		vp     += mt->alloc_sz;
		p->next = (MTBLANK *) vp;
		p       = p->next;
	}

	// and attach to the free list (it might not be null)
	p->next = mt->flist;

	// and update our type
	mt->flist   = list;
	mt->fcount += count;
	mt->total  += count;
}


inline void *__mtype_new( MTYPE *mt )
{
	MTBLANK *b;

	pthread_mutex_lock( &(mt->lock) );

	if( !mt->fcount )
		__mtype_alloc_free( mt, 0 );

	b         = mt->flist;
	mt->flist = b->next;
	--(mt->fcount);

	pthread_mutex_unlock( &(mt->lock) );

	b->next = NULL;

	return (void *) b;
}

inline void *__mtype_new_list( MTYPE *mt, int count )
{
	MTBLANK *top, *end;
	int i;

	if( count <= 0 )
		return NULL;

	pthread_mutex_lock( &(mt->lock) );

	// get enough
	while( mt->fcount < count )
		__mtype_alloc_free( mt, 0 );

	top = end = mt->flist;

	// run down count - 1 elements
	for( i = count - 1; i > 0; i-- )
		end = end->next;

	// end is now the last in the list we want
	mt->flist   = end->next;
	mt->fcount -= count;

	pthread_mutex_unlock( &(mt->lock) );

	end->next = NULL;

	return (void *) top;
}



inline void __mtype_free( MTYPE *mt, void *p )
{
	MTBLANK *b = (MTBLANK *) p;

	pthread_mutex_lock( &(mt->lock) );

	b->next   = mt->flist;
	mt->flist = p;
	++(mt->fcount);

	pthread_mutex_unlock( &(mt->lock) );
}


inline void __mtype_free_list( MTYPE *mt, int count, void *first, void *last )
{
	MTBLANK *l = (MTBLANK *) last;

	pthread_mutex_lock( &(mt->lock) );

	l->next     = mt->flist;
	mt->flist   = first;
	mt->fcount += count;

	pthread_mutex_unlock( &(mt->lock) );
}





HOST *mem_new_host( void )
{
	HOST *h;

	h = (HOST *) __mtype_new( ctl->mem->hosts );

	// is this one set up?
	if( ! h->net )
	{
		h->net = net_make_sock( MIN_NETBUF_SZ, MIN_NETBUF_SZ,
		                        NULL, &(h->peer) );
		h->all = (WORDS *) allocz( sizeof( WORDS ) );
		h->val = (WORDS *) allocz( sizeof( WORDS ) );
	}

	return h;
}

void mem_free_host( HOST **h )
{
	HOST *sh;

	if( !h || !*h )
		return;

	sh = *h;
	*h = NULL;

	sh->points         = 0;
	sh->type           = NULL;
	sh->last           = 0;
	sh->started        = 0;
	sh->net->sock      = -1;
	sh->net->flags     = 0;
	sh->net->out->len  = 0;
	sh->net->in->len   = 0;
	sh->net->keep->len = 0;
	sh->net->keep->buf = NULL;

	if( sh->net->name )
	{
		free( sh->net->name );
		sh->net->name = NULL;
	}

	__mtype_free( ctl->mem->hosts, sh );
}



PTLIST *mem_new_point( void )
{
	return (PTLIST *) __mtype_new( ctl->mem->points );
}

void mem_free_point( PTLIST **p )
{
	PTLIST *sp;

	if( !p || !*p )
		return;

	sp = *p;
	*p = NULL;

	sp->count = 0;

	__mtype_free( ctl->mem->points, sp );
}

void mem_free_point_list( PTLIST *list )
{
	PTLIST *p, *freed, *end;
	int j = 0;

	freed = end = NULL;

	while( list )
	{
		p    = list;
		list = p->next;

		p->count = 0;
		p->next  = freed;
		freed    = p;

		if( !end )
			end = p;

		j++;
	}

	__mtype_free_list( ctl->mem->points, j, freed, end );
}



DHASH *mem_new_dhash( char *str, int len, int type )
{
	DHASH *d = __mtype_new( ctl->mem->dhash );

	d->type = type;

	if( len >= d->sz )
	{
		free( d->path );
		d->sz   = len + 1;
		d->path = (char *) allocz( d->sz );
	}

	// copy the string
	memcpy( d->path, str, len );
	d->path[len] = '\0';
	d->len = len;

	return d;
}

void mem_free_dhash( DHASH **d )
{
	DHASH *sd;

	if( !d || !*d )
		return;

	sd = *d;
	*d = NULL;

	*(sd->path) = '\0';
	sd->len     = 0;

	if( sd->type == DATA_HTYPE_STATS && sd->in.points )
	{
		mem_free_point_list( sd->in.points );
		sd->in.points = NULL;
	}
	else
	{
		sd->in.sum.total = 0;
		sd->in.sum.count = 0;
	}

	sd->type = 0;

	__mtype_free( ctl->mem->dhash, sd );
}

void mem_free_dhash_list( DHASH *list )
{
	DHASH *d, *freed, *end;
	PTLIST *ptfree, *p;
	int j = 0;

	freed = end = NULL;
	ptfree = NULL;

	while( list )
	{
		d    = list;
		list = d->next;

		*(d->path) = '\0';
		d->len     = 0;

		if( d->type == DATA_HTYPE_STATS && d->in.points )
		{
			for( p = d->in.points; p->next; p = p->next );

			p->next = ptfree;
			ptfree  = d->in.points;

			d->in.points = NULL;
		}
		else
		{
			d->in.sum.total = 0;
			d->in.sum.count = 0;
		}



		d->next  = freed;
		freed    = d;

		if( !end )
			end = d;

		j++;
	}

	__mtype_free_list( ctl->mem->dhash, j, freed, end );

	if( ptfree )
		mem_free_point_list( ptfree );
}



IOLIST *mem_new_iolist( void )
{
	return __mtype_new( ctl->mem->iolist );
}

void mem_free_iolist( IOLIST **l )
{
	IOLIST *sl;

	if( !l || !*l )
		return;

	sl = *l;
	*l = NULL;

	sl->prev = NULL;
	sl->buf  = NULL;

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

		l->buf  = NULL;
		l->prev = NULL;

		l->next = freed;
		freed   = l;

		if( !end )
			end = l;

		j++;
	}

	__mtype_free_list( ctl->mem->iolist, j, freed, end );
}


IOBUF *mem_new_buf( int sz )
{
	IOBUF *b;

	// dangerous returning one with memory attached
	// if not asked for it
	if( sz == 0 )
		return (IOBUF *) allocz( sizeof( IOBUF ) );

	b = (IOBUF *) __mtype_new( ctl->mem->iobufs );

	if( sz < 0 )
		sz = MIN_NETBUF_SZ;

	if( b->sz < sz )
	{
		if( b->buf )
			free( b->buf );

		b->buf = (char *) allocz( sz );
		b->sz  = sz;
	}

	b->hwmk = b->buf + ( ( 5 * b->sz ) / 6 );

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
		sb->buf[0] = '\0';
	else
	{
		sb->buf = sb->hwmk = NULL;
	}

	sb->len = 0;

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
			b->buf[0] = '\0';
		else
		{
			b->buf = b->hwmk = NULL;
		}

		b->len  = 0;
		b->next = freed;
		freed   = b;

		if( !end )
			end = b;

		j++;
	}

	__mtype_free_list( ctl->mem->iobufs, j, freed, end );
}



static int mem_check_counter = 0;
static int mem_check_max     = 1;

void mem_check( uint64_t tval, void *arg )
{
	struct rusage ru;

	// we only do this every so often, but we need the faster
	// loop for responsiveness to shutdown
	if( ++mem_check_counter < mem_check_max )
		return;

	mem_check_counter = 0;

	getrusage( RUSAGE_SELF, &ru );

	ctl->mem->curr_kb = ru.ru_maxrss;

	if( ctl->mem->curr_kb > ctl->mem->max_kb )
		loop_end( "Memory usage exceeds configured maximum." );
}


void *mem_loop( void *arg )
{
	THRD *t = (THRD *) arg;
	int usec;

	usec = 1000 * ctl->mem->interval;

	// don't make us wait too long
	if( usec > 3000000 )
	{
		mem_check_max = 10;
		usec /= 10;
	}

	loop_control( "memory control", mem_check, NULL, usec, 0, 0 );

	free( t );
	return NULL;
}



int mem_config_line( AVP *av )
{
	MTYPE *mt;
	char *d;
	int t;

	if( !( d = strchr( av->att, '.' ) ) )
	{
		// just the singles
		if( attIs( "max_mb" ) || attIs( "max_size" ) )
			ctl->mem->max_kb = 1024 * atoi( av->val );
		else if( attIs( "max_kb" ) )
			ctl->mem->max_kb = atoi( av->val );
		else if( attIs( "interval" ) || attIs( "msec" ) )
			ctl->mem->interval = atoi( av->val );
		else if( attIs( "hashsize" ) )
			ctl->mem->hashsize = atoi( av->val );
		else if( attIs( "gc_thresh" ) )
		{
			t = atoi( av->val );
			if( !t )
				t = DEFAULT_GC_THRESH;
			info( "Garbage collection threshold set to %d stats intervals.", t );
			ctl->mem->gc_thresh = t;
		}
		else
			return -1;

		return 0;
	}

	*d++ = '\0';

	// after this, it's per-type control
	if( !strncasecmp( av->att, "hosts.", 6 ) )
		mt = ctl->mem->hosts;
	else if( !strncasecmp( av->att, "iobufs.", 7 ) )
		mt = ctl->mem->iobufs;
	else if( !strncasecmp( av->att, "points.", 7 ) )
		mt = ctl->mem->points;
	else if( !strncasecmp( av->att, "dhash.", 6 ) )
		mt = ctl->mem->dhash;
	else if( !strncasecmp( av->att, "iolist.", 7 ) )
		mt = ctl->mem->iolist;
	else
		return -1;

	if( !strcasecmp( d, "block" ) )
	{
		mt->alloc_ct = (uint16_t) atoi( av->val );
		info( "Allocation block for %s set to %hu", av->att, mt->alloc_ct );
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

	pthread_mutex_destroy( &(m->hosts->lock)  );
	pthread_mutex_destroy( &(m->points->lock) );
	pthread_mutex_destroy( &(m->iobufs->lock) );
	pthread_mutex_destroy( &(m->dhash->lock)  );
	pthread_mutex_destroy( &(m->iolist->lock) );
}


MTYPE *__mem_type_ctl( int sz, int ct )
{
	MTYPE *mt;

	mt           = (MTYPE *) allocz( sizeof( MTYPE ) );
	mt->alloc_sz = sz;
	mt->alloc_ct = ct;

	// and alloc some already
	__mtype_alloc_free( mt, mt->alloc_ct );

	// init the mutex
	pthread_mutex_init( &(mt->lock), NULL );

	return mt;
}



MEM_CTL *mem_config_defaults( void )
{
	MEM_CTL *m;

	m = (MEM_CTL *) allocz( sizeof( MEM_CTL ) );

	m->hosts     = __mem_type_ctl( sizeof( HOST ),   MEM_ALLOCSZ_HOSTS  );
	m->iobufs    = __mem_type_ctl( sizeof( IOBUF ),  MEM_ALLOCSZ_IOBUF  );
	m->points    = __mem_type_ctl( sizeof( PTLIST ), MEM_ALLOCSZ_POINTS );
	m->dhash     = __mem_type_ctl( sizeof( DHASH ),  MEM_ALLOCSZ_DHASH  );
	m->iolist    = __mem_type_ctl( sizeof( IOLIST ), MEM_ALLOCSZ_IOLIST );

	m->max_kb    = DEFAULT_MEM_MAX_KB;
	m->interval  = DEFAULT_MEM_CHECK_INTV;
	m->gc_thresh = DEFAULT_GC_THRESH;
	m->hashsize  = DEFAULT_MEM_HASHSIZE;

	return m;
}

