#include "ministry.h"


HOST *mem_new_host( void )
{
	HOST *h;

	pthread_mutex_lock( &(ctl->locks->hostalloc) );
	if( ctl->mem->hosts )
	{
		h = ctl->mem->hosts;
		ctl->mem->hosts = h->next;
		--(ctl->mem->free_hosts);
		pthread_mutex_unlock( &(ctl->locks->hostalloc) );

		h->next = NULL;
	}
	else
	{
		++(ctl->mem->mem_hosts);
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

	sh->points        = 0;
	sh->last          = 0;
	sh->started       = 0;
	sh->net->sock     = -1;
	sh->net->flags    = 0;
	sh->net->out->len = 0;
	sh->net->in->len  = 0;

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
	++(ctl->mem->free_hosts);

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
void __mem_new_dstat( int count )
{
    DSTAT *dlist, *d;
    int i;

    // allocate a block of paths and words
    dlist = (DSTAT *) allocz( count * sizeof( DSTAT ) );

    d = dlist + 1;

    // link them up
    for( i = 0; i < count; i++ )
        dlist[i].next = d++;

    // and insert them
    dlist[count - 1].next = ctl->mem->dstat;
    ctl->mem->dstat = dlist;
    ctl->mem->free_dstat += count;
    ctl->mem->mem_dstat  += count;
}



DSTAT *mem_new_dstat( char *str, int len )
{
	DSTAT *d;

	pthread_mutex_lock( &(ctl->locks->statalloc) );

	if( !ctl->mem->dstat )
        __mem_new_dstat( NEW_DSTAT_BLOCK_SZ );

	d = ctl->mem->dstat;
	ctl->mem->dstat = d->next;
	--(ctl->mem->free_dstat);

	pthread_mutex_unlock( &(ctl->locks->statalloc) );

	d->next = NULL;

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


void mem_free_dstat( DSTAT **d )
{
	DSTAT *sd;

	if( !d || !*d )
		return;

	sd = *d;
	*d = NULL;

	*(sd->path) = '\0';
	sd->len     = 0;

	if( sd->points )
	{
		mem_free_point_list( sd->points );
		sd->points = NULL;
	}

	// ignore processing, it gets tidied up anyway

	pthread_mutex_lock( &(ctl->locks->statalloc) );

	sd->next = ctl->mem->dstat;
	ctl->mem->dstat = sd;
	++(ctl->mem->free_dstat);

	pthread_mutex_unlock( &(ctl->locks->statalloc) );
}


// this must always be called from inside a lock
void __mem_new_dadd( int count )
{
    DADD *dlist, *d;
    int i;

    // allocate a block of paths and words
    dlist = (DADD *) allocz( count * sizeof( DADD ) );

    d = dlist + 1;

    // link them up
    for( i = 0; i < count; i++ )
        dlist[i].next = d++;

    // and insert them
    dlist[count - 1].next = ctl->mem->dadd;
    ctl->mem->dadd = dlist;
    ctl->mem->free_dadd += count;
    ctl->mem->mem_dadd  += count;
}




DADD *mem_new_dadd( char *str, int len )
{
	DADD *d;

	pthread_mutex_lock( &(ctl->locks->addalloc) );

	if( !ctl->mem->dadd )
        __mem_new_dadd( NEW_DADD_BLOCK_SZ );

	d = ctl->mem->dadd;
	ctl->mem->dadd = d->next;
	--(ctl->mem->free_dadd);

	pthread_mutex_unlock( &(ctl->locks->addalloc) );

	d->next = NULL;

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

void mem_free_dadd( DADD **d )
{
	DADD *sd;

	if( !d || !*d )
		return;

	sd = *d;
	*d = NULL;

	*(sd->path) = '\0';
	sd->len     = 0;

	pthread_mutex_lock( &(ctl->locks->addalloc) );

	sd->next = ctl->mem->dadd;
	ctl->mem->dadd = sd;
	++(ctl->mem->free_dadd);

	pthread_mutex_unlock( &(ctl->locks->addalloc) );
}



void mem_check( void *arg )
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

	m->max_mb = MEM_MAX_MB;

	return m;
}

int mem_config_line( AVP *av )
{
	if( attIs( "max_mb" ) )
		ctl->mem->max_mb = 1024 * atoi( av->val );
	else
		return -1;

	return 0;
}


