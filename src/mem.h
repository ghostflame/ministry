/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem.h - defines main memory control structures                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef MINISTRY_MEM_H
#define MINISTRY_MEM_H

#define DEFAULT_MEM_MAX_KB			( 10 * 1024 * 1024 )
#define DEFAULT_MEM_CHECK_INTV		5000		// msec
#define DEFAULT_MEM_PRE_INTV		50			// msec
#define DEFAULT_MEM_STACK_SIZE		128			// gets converted to KB

#define DEFAULT_MEM_PRE_THRESH		0.33

#define MEM_ALLOCSZ_HOSTS			128
#define MEM_ALLOCSZ_POINTS			512
#define MEM_ALLOCSZ_DHASH			512
#define MEM_ALLOCSZ_IOBUF			128
#define MEM_ALLOCSZ_IOLIST			512
#define MEM_ALLOCSZ_TOKEN			128

#define DEFAULT_GC_THRESH			8640		// 1 day @ 10s
#define DEFAULT_GC_GG_THRESH		25920		// 3 days @ 10s


// pre-defined hash sizes
#define MEM_HSZ_TINY				1009
#define MEM_HSZ_SMALL				5003
#define MEM_HSZ_MEDIUM				25013
#define MEM_HSZ_LARGE				100003
#define MEM_HSZ_XLARGE				425071
#define MEM_HSZ_X2LARGE				1300021


// universal type for memory management
// means we can only manage things that start
// with a next pointer
struct mem_type_blank
{
	MTBLANK			*	next;
};


struct mem_type_control
{
	MTBLANK			*	flist;
	char			*	name;

	uint32_t			fcount;
	uint32_t			total;

	uint32_t			alloc_sz;
	uint32_t			alloc_ct;

	uint32_t			stats_sz;
	uint32_t			prealloc;

	double				threshold;

	pthread_mutex_t		lock;
};


struct mem_control
{
	MTYPE			*	hosts;
	MTYPE			*	points;
	MTYPE			*	dhash;
	MTYPE			*	iobufs;
	MTYPE			*	iolist;
	MTYPE			*	token;

	int64_t				curr_kb;
	int64_t				max_kb;
	int64_t				hashsize;
	int64_t				stacksize;
	int64_t				interval;	// msec
	int64_t				prealloc;	// msec

	int64_t				gc_enabled;
	int64_t				gc_thresh;
	int64_t				gc_gg_thresh;
};


void *mem_reverse_list( void *list_in );

HOST *mem_new_host( struct sockaddr_in *peer, uint32_t bufsz );
void mem_free_host( HOST **h );

PTLIST *mem_new_point( void );
void mem_free_point( PTLIST **p );
void mem_free_point_list( PTLIST *list );

IOLIST *mem_new_iolist( void );
void mem_free_iolist( IOLIST **l );
void mem_free_iolist_list( IOLIST *list );

DHASH *mem_new_dhash( char *str, int len );
void mem_free_dhash( DHASH **d );
void mem_free_dhash_list( DHASH *list );

IOBUF *mem_new_buf( int sz );
void mem_free_buf( IOBUF **b );
void mem_free_buf_list( IOBUF *list );

TOKEN *mem_new_token( void );
void mem_free_token( TOKEN **t );
void mem_free_token_list( TOKEN *list );

loop_call_fn mem_prealloc;
throw_fn mem_prealloc_loop;

loop_call_fn mem_check;
throw_fn mem_check_loop;

void mem_shutdown( void );
int mem_config_line( AVP *av );
MEM_CTL *mem_config_defaults( void );


#endif
