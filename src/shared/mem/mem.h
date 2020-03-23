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


struct mem_hanger
{
	MEMHG				*	next;
	MEMHG				*	prev;
	void				*	ptr;
	MEMHL				*	list;
};

struct mem_hanger_list
{
	MEMHL				*	next;
	MEMHG				*	head;
	MEMHG				*	tail;
	mem_free_cb			*	memcb;
	int64_t					count;
	uint64_t				id;

	pthread_mutex_t			lock;
	int8_t					use_lock;	// use locking for updates
	int8_t					act_lock;	// 1 when lockable, 0 when locked
};


struct mem_call_counters
{
	int64_t					ctr;
	int64_t					sum;
};

struct mem_type_counters
{
	uint32_t				fcount;
	uint32_t				total;

#ifdef MTYPE_TRACING
	MCCTR					all;
	MCCTR					fre;
	MCCTR					pre;
	MCCTR					ref;
#endif
};


struct mem_type_stats
{
	char				*	name;
	uint64_t				bytes;

	MTCTR					ctrs;
};

struct mem_control
{
	MTYPE				*	types[MEM_TYPES_MAX];

	MCHK				*	mcheck;

	int64_t					prealloc;	// msec
	int16_t					type_ct;
	pthread_mutex_t			idlock;
	uint64_t				id;

	pthread_mutexattr_t		mtxa;

	PERM				*	perm;		// permie string space

	// known types
	MTYPE				*	iobufs;
	MTYPE				*	htreq;
	MTYPE				*	htprm;
	MTYPE				*	hosts;
	MTYPE				*	token;
	MTYPE				*	hanger;
	MTYPE				*	slkmsg;
	MTYPE				*	store;
	MTYPE				*	ptser;
};


// memory management
uint32_t mem_alloc_size( int len );

// zero'd memory
void *allocz( size_t size );
void *mem_perm( uint32_t len );

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

HTREQ *mem_new_request( void );
void mem_free_request( HTREQ **h );

HTPRM *mem_new_htprm( void );
void mem_free_htprm( HTPRM **a );
void mem_free_htprm_list( HTPRM *list );

HOST *mem_new_host( struct sockaddr_in *peer, uint32_t bufsz );
void mem_free_host( HOST **h );

TOKEN *mem_new_token( void );
void mem_free_token( TOKEN **t );
void mem_free_token_list( TOKEN *list );

MEMHG *mem_new_hanger( void *ptr, MEMHL *list );
void mem_free_hanger( MEMHG **m );
void mem_free_hanger_list( MEMHG *list );

SLKMSG *mem_new_slack_msg( size_t sz );
void mem_free_slack_msg( SLKMSG **m );

SSTE *mem_new_store( void );
void mem_free_store( SSTE **s );
void mem_free_store_list( SSTE *list );

PTL *mem_new_ptser( int count );
void mem_free_ptser( PTL **p );
void mem_free_ptser_list( PTL *list );

// hanger list

MEMHL *mem_list_filter_new( MEMHL *mhl, void *arg, mhl_callback *cb );
int mem_list_filter_self( MEMHL *mhl, void *arg, mhl_callback *cb );
int mem_list_iterator( MEMHL *mhl, void *arg, mhl_callback *cb );
MEMHG *mem_list_search( MEMHL *mhl, void *arg, mhl_callback *cb );
MEMHG *mem_list_find( MEMHL *mhl, void *ptr );

// add/fetch void ptrs to your objects
int mem_list_remove( MEMHL *mhl, MEMHG *hg );
int mem_list_excise( MEMHL *mhl, void *ptr );
void mem_list_add_tail( MEMHL *mhl, void *ptr );
void mem_list_add_head( MEMHL *mhl, void *ptr );
void *mem_list_get_head( MEMHL *mhl );
void *mem_list_get_tail( MEMHL *mhl );

// external locking - disables internal locking!
int mem_list_lock( MEMHL *mhl );
int mem_list_unlock( MEMHL *mhl );

// info
int64_t mem_list_size( MEMHL *mhl );

// setup/teardown
void mem_list_free( MEMHL *mhl );
MEMHL *mem_list_create( int use_lock, mem_free_cb *cb );

#endif
