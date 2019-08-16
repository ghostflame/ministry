/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem/mem.c - memory control functions and utilities                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



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
	for( i = 0; i < j; ++i )
		tmp[i]->next = tmp[i+1];

	tmp[j]->next = NULL;
	*list = tmp[0];

	free( tmp );
}

int mem_cmp_dbl( const void *p1, const void *p2 )
{
	double d1 = *((double *) p1);
	double d2 = *((double *) p2);

	return ( d1 > d2 ) ? 1 : ( d1 == d2 ) ? 0 : -1;
}

int mem_cmp_i64( const void *p1, const void *p2 )
{
	int64_t i1 = *((int64_t *) p1);
	int64_t i2 = *((int64_t *) p2);

	return ( i1 > i2 ) ? 1 : ( i1 == i2 ) ? 0 : -1;
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

	m->proc_kb = strtol( m->w->wd[23], NULL, 10 ) * m->psize;
	//info( "/proc/self/stat[23] reports %d KB", m->proc_kb );

	// get our virtual size - comes in bytes, turn into kb
	m->virt_kb = strtol( m->w->wd[22], NULL, 10 ) >> 10;

	// use the higher of the two
	m->curr_kb = ( m->rusage_kb > m->proc_kb ) ? m->rusage_kb : m->proc_kb;

	if( m->curr_kb > m->max_kb )
	{
		warn( "Memory usage (%ld KB) exceeds configured maximum (%ld KB), exiting.", m->curr_kb, m->max_kb );
		loop_end( "Memory usage exceeds configured maximum." );
	}
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




// shut down memory locks
void mem_shutdown( void )
{
	MTYPE *mt;
	int i;

	for( i = 0; i < MEM_TYPES_MAX; ++i )
	{
		if( !( mt = _mem->types[i] ) )
			break;

		pthread_mutex_destroy( &(mt->lock) );
	}
}

// do we do checks?
void mem_startup( void )
{
	//PMET *p, *q;
	//int i;

	if( _mem->mcheck->checks )
		thread_throw_named( &mem_check_loop, NULL, 0, "mem_check_loop" );

	thread_throw_named( &mem_prealloc_loop, NULL, 0, "mem_prealloc" );

	//if( !( p = pmet_create( PMET_TYPE_GAUGE,
	//                "mem_allocation", "Amount of memory allocated",
	//                PMET_GEN_FN, 
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

// allow apps to override the default max 10
// As long as it wasn't set in config
void mem_set_max_kb( int64_t kb )
{
	if( _mem->mcheck->max_set == 0 )
		_mem->mcheck->max_kb = kb;
}


