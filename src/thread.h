/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* thread.h - threading structures, mutexes and locking macros             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_THREAD_H
#define MINISTRY_THREAD_H

#define HASHT_MUTEX_COUNT		0x10	// 16
#define HASHT_MUTEX_MASK		0x0f

#define DSTATS_MUTEX_COUNT		0x40	// 64
#define DSTATS_MUTEX_MASK		0x3f

#define DADDER_MUTEX_COUNT		0x40	// 64
#define DADDER_MUTEX_MASK		0x3f


#define lock_adder( d )			pthread_mutex_lock(   &(ctl->locks->dadder[d->id & DADDER_MUTEX_MASK]) )
#define unlock_adder( d )		pthread_mutex_unlock( &(ctl->locks->dadder[d->id & DADDER_MUTEX_MASK]) )

#define lock_stats( d )			pthread_mutex_lock(   &(ctl->locks->dstats[d->id & DSTATS_MUTEX_MASK]) )
#define unlock_stats( d )		pthread_mutex_unlock( &(ctl->locks->dstats[d->id & DSTATS_MUTEX_MASK]) )

#define lock_table( idx )		pthread_mutex_lock(   &(ctl->locks->table[idx & HASHT_MUTEX_MASK]) )
#define unlock_table( idx )		pthread_mutex_unlock( &(ctl->locks->table[idx & HASHT_MUTEX_MASK]) )

#define lock_target( t )		pthread_mutex_lock(   &(t->lock) )
#define unlock_target( t )		pthread_mutex_unlock( &(t->lock) )



struct lock_control
{
	pthread_mutex_t			hostalloc;					// mem hosts
	pthread_mutex_t			hashalloc;					// control of dhash allocation
	pthread_mutex_t			pointalloc;					// point list structures
	pthread_mutex_t			bufalloc;					// buffer structures
	pthread_mutex_t			listalloc;					// buflist structures

	pthread_mutex_t			hashstats;					// used for counters
	pthread_mutex_t			loop;						// thread startup/shutdown
	pthread_mutex_t			bufref;						// controlled buffer refcount

	pthread_mutex_t			table[HASHT_MUTEX_COUNT];	// hash table locks
	pthread_mutex_t			dstats[DSTATS_MUTEX_COUNT];	// path stat data
	pthread_mutex_t			dadder[DADDER_MUTEX_COUNT];	// path add data

	int						init_done;
};



struct thread_data
{
	THRD				*	next;
	pthread_t				id;
	void				*	arg;
	throw_fn			*	fp;		// only used for networks
};



pthread_t thread_throw( void *(*fp) (void *), void *arg );
void thread_throw_watched( void *(*fp) (void *), void *arg );

LOCK_CTL *lock_config_defaults( void );
void lock_shutdown( void );


#endif
