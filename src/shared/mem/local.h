/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem/local.h - defines local memory control structures                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef SHARED_MEM_LOCAL_H
#define SHARED_MEM_LOCAL_H

#define DEFAULT_MEM_MAX_KB			( 10 * 1024 * 1024 )
#define DEFAULT_MEM_CHECK_INTV		5000		// msec
#define DEFAULT_MEM_PRE_INTV		50			// msec

#define DEFAULT_MEM_PRE_THRESH		0.33

#define PERM_SPACE_BLOCK			0x100000   // 1M

// and some types
#define MEM_ALLOCSZ_IOBUF			170		// 8k
#define MEM_ALLOCSZ_HTREQ			128
#define MEM_ALLOCSZ_HTPRM			128
#define MEM_ALLOCSZ_HOSTS			128
#define MEM_ALLOCSZ_TOKENS			128
#define MEM_ALLOCSZ_HANGER			1024
#define MEM_ALLOCSZ_SLKMSG			16
#define MEM_ALLOCSZ_STORE			819		// 32k
#define MEM_ALLOCSZ_PTSER			1024


// keep points on a PTL if it is less than this
#define MEM_PTSER_MAX_KEEP_POINTS	3601


#define mem_lock( mt )			pthread_mutex_lock(   &(mt->lock) )
#define mem_unlock( mt )		pthread_mutex_unlock( &(mt->lock) )


#define mhl_lock( _m )			if( _m->use_lock && _m->act_lock ) pthread_mutex_lock(   &(_m->lock) )
#define mhl_unlock( _m )		if( _m->use_lock && _m->act_lock ) pthread_mutex_unlock( &(_m->lock) )



#include "shared.h"


// universal type for memory management
// means we can only manage things that start
// with a next pointer
struct mem_type_blank
{
	MTBLANK			*	next;
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


struct mem_perm
{
	void			*	space;
	void			*	curr;
	uint32_t			size;
	uint32_t			left;

	pthread_mutex_t		lock;
};



loop_call_fn mem_prealloc;
throw_fn mem_prealloc_loop;

loop_call_fn mem_check;
throw_fn mem_check_loop;


uint64_t mem_get_id( void );


extern MEM_CTL *_mem;


#endif
