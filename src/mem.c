#include "ministry.h"


HOST *mem_new_host( void )
{
	HOST *h;

	pthread_mutex_lock( &(ctl->locks->hostalloc) );
	if( ctl->mem->hosts )
	{
		h = ctl->mem->hosts;
		ctl->mem->hosts = h->next;
		ctl->mem->free_hosts--;
		pthread_mutex_unlock( &(ctl->locks->hostalloc) );

		h->next = NULL;
	}
	else
	{
		ctl->mem->mem_hosts++;
		pthread_mutex_unlock( &(ctl->locks->hostalloc) );

		h      = (HOST *) allocz( sizeof( HOST ) );
		h->net = net_make_sock( MIN_NETBUF_SZ, MIN_NETBUF_SZ, NULL, &(h->peer) );
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
	sh->net->keep->buf = NULL;
	sh->net->keep->len = 0;

	if( sh->net->name )
	{
		free( sh->net->name );
		sh->net->name = NULL;
	}

	pthread_mutex_lock( &(ctl->locks->hostalloc) );

	sh->next = ctl->mem->hosts;
	ctl->mem->hosts = sh;
	ctl->mem->free_hosts++;

	pthread_mutex_unlock( &(ctl->locks->hostalloc) );
}



// this must always be called inside a lock
void __mem_new_points( int count )
{
    PTLIST *list, *p;
    int i;

    // allocate a block of points
    list = (PTLIST *) allocz( count * sizeof( PTLIST ) );

    p = list + 1;

    // link them up
    for( i = 0; i < count; i++ )
        list[i].next = p++;

    // and insert them
    list[count - 1].next = ctl->mem->points;
    ctl->mem->points  = list;
    ctl->mem->free_points += count;
    ctl->mem->mem_points  += count;
}



PTLIST *mem_new_point( void )
{
	PTLIST *p;

	pthread_mutex_lock( &(ctl->locks->pointalloc) );

    if( !ctl->mem->points )
        __mem_new_points( NEW_PTLIST_BLOCK_SZ );

	p = ctl->mem->points;
	ctl->mem->points = p->next;
	--(ctl->mem->free_points);

	pthread_mutex_unlock( &(ctl->locks->pointalloc) );

	p->next = NULL;

	return p;
}


void mem_free_point( PTLIST **p )
{
	PTLIST *sp;

	if( !p || !*p )
		return;

	sp = *p;
	*p = NULL;

	memset( sp, 0, sizeof( PTLIST ) );

	pthread_mutex_lock( &(ctl->locks->pointalloc) );

	sp->next = ctl->mem->points;
	ctl->mem->points = sp;
	++(ctl->mem->free_points);

	pthread_mutex_unlock( &(ctl->locks->pointalloc) );
}

void mem_free_point_list( PTLIST *list )
{
	PTLIST *freed, *p, *end;
	int j = 0;

	freed = end = NULL;

	while( list )
	{
		p    = list;
		list = p->next;

		memset( p, 0, sizeof( PTLIST ) );
		p->next = freed;
		freed   = p;

		if( !end )
			end = p;

		j++;
	}

	pthread_mutex_lock( &(ctl->locks->pointalloc) );

	end->next = ctl->mem->points;
	ctl->mem->points = freed;
	ctl->mem->free_points += j;

	pthread_mutex_unlock( &(ctl->locks->pointalloc) );
}


// this must always be called from inside a lock
void __mem_new_dhash( int count )
{
    DHASH *dlist, *d;
    int i;

    // allocate a block of paths and words
    dlist = (DHASH *) allocz( count * sizeof( DHASH ) );

    d = dlist + 1;

    // link them up
    for( i = 0; i < count; i++ )
        dlist[i].next = d++;

    // and insert them
    dlist[count - 1].next = ctl->mem->dhash;
    ctl->mem->dhash = dlist;
    ctl->mem->free_dhash += count;
    ctl->mem->mem_dhash  += count;
}



DHASH *mem_new_dhash( char *str, int len, int type )
{
	DHASH *d;

	pthread_mutex_lock( &(ctl->locks->hashalloc) );

	if( !ctl->mem->dhash )
        __mem_new_dhash( NEW_DHASH_BLOCK_SZ );

	d = ctl->mem->dhash;
	ctl->mem->dhash = d->next;
	--(ctl->mem->free_dhash);

	pthread_mutex_unlock( &(ctl->locks->hashalloc) );

	d->next = NULL;
	d->type = type;

	if( len >= d->sz )
	{
	  	free( d->path );
		d->sz   = len + 1;
		d->path = (char *) allocz( d->sz );
	}

	// copy the string into place
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
		sd->in.total = 0;

	sd->type = 0;

	// ignore processing, it gets tidied up anyway

	pthread_mutex_lock( &(ctl->locks->hashalloc) );

	sd->next = ctl->mem->dhash;
	ctl->mem->dhash = sd;
	++(ctl->mem->free_dhash);

	pthread_mutex_unlock( &(ctl->locks->hashalloc) );
}


void mem_free_dhash_list( DHASH *list )
{
	DHASH *freed, *d, *end;
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
			ptfree  = p;

			d->in.points = NULL;
		}
		else
			d->in.total = 0;

		d->next = freed;
		freed   = d;

		if( !end )
			end = d;

		j++;
	}

	pthread_mutex_lock( &(ctl->locks->hashalloc) );

	end->next = ctl->mem->dhash;
	ctl->mem->dhash = d;
	ctl->mem->free_dhash += j;

	pthread_mutex_unlock( &(ctl->locks->hashalloc) );

	if( ptfree )
		mem_free_point_list( ptfree );
}



IOBUF *mem_new_buf( int sz )
{
	IOBUF *b;

	// dangerous returning one with memory attached
	if( sz == 0 )
		return (IOBUF *) allocz( sizeof( IOBUF ) );


	pthread_mutex_lock( &(ctl->locks->bufalloc) );
	if( ctl->mem->bufs )
	{
		b = ctl->mem->bufs;
		ctl->mem->bufs = b->next;
		--(ctl->mem->free_bufs);
		pthread_mutex_unlock( &(ctl->locks->bufalloc) );

		b->next = NULL;
	}
	else
	{
		++(ctl->mem->mem_bufs);
		pthread_mutex_unlock( &(ctl->locks->bufalloc) );


		b = (IOBUF *) allocz( sizeof( IOBUF ) );
	}

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
		sb->buf  = NULL;
		sb->hwmk = NULL;
	}

	sb->len = 0;

	pthread_mutex_lock( &(ctl->locks->bufalloc) );

	sb->next = ctl->mem->bufs;
	ctl->mem->bufs = sb;
	++(ctl->mem->free_bufs);

	pthread_mutex_unlock( &(ctl->locks->bufalloc) );
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
			b->buf  = NULL;
			b->hwmk = NULL;
		}

		b->len  = 0;
		b->next = freed;
		freed   = b;

		if( !end )
			end = b;

		j++;
	}

	pthread_mutex_lock( &(ctl->locks->bufalloc) );

	end->next = ctl->mem->bufs;
	ctl->mem->bufs = freed;
	ctl->mem->free_bufs += j;

	pthread_mutex_unlock( &(ctl->locks->bufalloc) );
}




void mem_check( unsigned long long tval, void *arg )
{
	struct rusage ru;

	getrusage( RUSAGE_SELF, &ru );

	//debug( "Checking memory size against maximum: %d vs %d KB",
	//		ru.ru_maxrss, ctl->mem->max_mb );

	if( ru.ru_maxrss > ctl->mem->max_mb )
		loop_end( "Memory usage exceeds configured maximum." );
}




void *mem_loop( void *arg )
{
	THRD *t = (THRD *) arg;

	loop_control( "memory control", &mem_check, NULL, 10000000, 0, 0 );

	free( t );
	return NULL;
}



MEM_CTL *mem_config_defaults( void )
{
	MEM_CTL *m;

	m = (MEM_CTL *) allocz( sizeof( MEM_CTL ) );

	m->max_mb    = MEM_MAX_MB;
	m->gc_thresh = DEFAULT_GC_THRESH;
	m->hashsize  = DEFAULT_MEM_HASHSIZE;

	return m;
}

int mem_config_line( AVP *av )
{
	int t;

	if( attIs( "max_mb" ) || attIs( "max_size" ) )
		ctl->mem->max_mb = 1024 * atoi( av->val );
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


