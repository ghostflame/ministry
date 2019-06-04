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


struct mem_type_stats
{
	char			*	name;
	uint64_t			bytes;

	MTCTR				ctrs;
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

sort_fn mem_cmp_dbl;
sort_fn mem_cmp_i64;
#define mem_sort_dlist( _l, _c )		qsort( _l, _c, sizeof( double ), mem_cmp_dbl )
#define mem_sort_ilist( _l, _c )		qsort( _l, _c, sizeof( int64_t ), mem_cmp_i64 )

void *mem_reverse_list( void *list_in );
void mem_sort_list( void **list, int count, sort_fn *cmp );


void *mtype_new( MTYPE *mt );
void *mtype_new_list( MTYPE *mt, int count );
void mtype_free( MTYPE *mt, void *p );
void mtype_free_list( MTYPE *mt, int count, void *first, void *last );

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
