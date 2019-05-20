/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem.h - defines main memory control structures                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#define DEFAULT_MEM_MAX_KB			( 10 * 1024 * 1024 )
#define DEFAULT_MEM_CHECK_INTV		5000		// msec
#define DEFAULT_MEM_PRE_INTV		50			// msec

#define DEFAULT_MEM_PRE_THRESH		0.33

// pre-defined hash sizes
#define MEM_HSZ_NANO				41
#define MEM_HSZ_MICRO				199
#define MEM_HSZ_TINY				1009
#define MEM_HSZ_SMALL				5003
#define MEM_HSZ_MEDIUM				25013
#define MEM_HSZ_LARGE				100003
#define MEM_HSZ_XLARGE				425071
#define MEM_HSZ_X2LARGE				1300021

#define MEM_TYPES_MAX				128

// and some types
#define MEM_ALLOCSZ_IOBUF			128
#define MEM_ALLOCSZ_IOBP			512
#define MEM_ALLOCSZ_HTREQ			128


#define mem_lock( mt )			pthread_mutex_lock(   &(mt->lock) )
#define mem_unlock( mt )		pthread_mutex_unlock( &(mt->lock) )



// universal type for memory management
// means we can only manage things that start
// with a next pointer
struct mem_type_blank
{
	MTBLANK			*	next;
};

struct mem_call_counters
{
	int64_t				ctr;
	int64_t				sum;
};

struct mem_type_counters
{
	uint32_t			fcount;
	uint32_t			total;

#ifdef MTYPE_TRACING
	MCCTR				all;
	MCCTR				fre;
	MCCTR				pre;
	MCCTR				ref;
#endif
};


struct mem_type
{
	MTBLANK			*	flist;
	char			*	name;

	MTCTR				ctrs;

	uint32_t			alloc_sz;
	uint32_t			alloc_ct;
	uint32_t			stats_sz;
	uint32_t			prealloc;


	int16_t				id;

	double				threshold;

	pthread_mutex_t		lock;
};


struct mem_type_stats
{
	char			*	name;
	uint64_t			bytes;

	MTCTR				ctrs;
};

struct mem_check
{
	char			*	buf;
	WORDS			*	w;
	int64_t				psize;
	int64_t				bsize;
	int64_t				checks;
	int64_t				interval;	// msec
	int64_t				curr_kb;
	int64_t				rusage_kb;
	int64_t				proc_kb;
	int64_t				virt_kb;
	int64_t				max_kb;
	int					max_set;
};

struct mem_control
{
	MTYPE			*	types[MEM_TYPES_MAX];

	MCHK			*	mcheck;

	int64_t				prealloc;	// msec
	int16_t				type_ct;

	// known types
	MTYPE			*	iobufs;
	MTYPE			*	iobps;
	MTYPE			*	htreq;
};


// memory management
uint32_t mem_alloc_size( int len );

// zero'd memory
void *allocz( size_t size );


void *mem_reverse_list( void *list_in );
void mem_sort_list( void **list, int count, sort_fn *cmp );


void *mtype_new( MTYPE *mt );
void *mtype_new_list( MTYPE *mt, int count );
void mtype_free( MTYPE *mt, void *p );
void mtype_free_list( MTYPE *mt, int count, void *first, void *last );

loop_call_fn mem_prealloc;
throw_fn mem_prealloc_loop;

loop_call_fn mem_check;
throw_fn mem_check_loop;

MTYPE *mem_type_declare( char *name, int sz, int ct, int extra, uint32_t pre );
int mem_type_stats( int id, MTSTAT *ms );
int64_t mem_curr_kb( void );
int64_t mem_virt_kb( void );
void mem_set_max_kb( int64_t kb );

void mem_shutdown( void );
void mem_startup( void );
MEM_CTL *mem_config_defaults( void );
conf_line_fn mem_config_line;


// types

IOBUF *mem_new_iobuf( int sz );
void mem_free_iobuf( IOBUF **b );
void mem_free_iobuf_list( IOBUF *list );

IOBP *mem_new_iobp( void );
void mem_free_iobp( IOBP **b );

HTREQ *mem_new_request( void );
void mem_free_request( HTREQ **h );


#endif
