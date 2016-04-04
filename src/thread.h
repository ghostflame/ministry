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

#define HASHT_MUTEX_COUNT		0x20	// 32
#define HASHT_MUTEX_MASK		0x1f

#define DSTATS_SLOCK_COUNT		0x80	// 128
#define DSTATS_SLOCK_MASK		0x7f

#define DADDER_SLOCK_COUNT		0x80	// 128
#define DADDER_SLOCK_MASK		0x7f

#define DGAUGE_SLOCK_COUNT		0x40	// 64
#define DGAUGE_SLOCK_MASK		0x3f



#define lock_adder( d )			pthread_spin_lock(   &(ctl->locks->dadder[d->id & DADDER_SLOCK_MASK]) )
#define unlock_adder( d )		pthread_spin_unlock( &(ctl->locks->dadder[d->id & DADDER_SLOCK_MASK]) )

#define lock_stats( d )			pthread_spin_lock(   &(ctl->locks->dstats[d->id & DSTATS_SLOCK_MASK]) )
#define unlock_stats( d )		pthread_spin_unlock( &(ctl->locks->dstats[d->id & DSTATS_SLOCK_MASK]) )

#define lock_gauge( d )			pthread_spin_lock(   &(ctl->locks->dgauge[d->id & DGAUGE_SLOCK_MASK]) )
#define unlock_gauge( d )		pthread_spin_unlock( &(ctl->locks->dgauge[d->id & DGAUGE_SLOCK_MASK]) )

#define lock_table( idx )		pthread_mutex_lock(   &(ctl->locks->table[idx & HASHT_MUTEX_MASK]) )
#define unlock_table( idx )		pthread_mutex_unlock( &(ctl->locks->table[idx & HASHT_MUTEX_MASK]) )

#define lock_target( t )		pthread_mutex_lock(   &(t->lock) )
#define unlock_target( t )		pthread_mutex_unlock( &(t->lock) )

#define lock_stthr( s )			lock_target( s )
#define unlock_stthr( s )		unlock_target( s )

#define lock_synth( )			pthread_mutex_lock(   &(ctl->locks->synth) )
#define unlock_synth( )			pthread_mutex_unlock( &(ctl->locks->synth) )

#define lock_mem( mt )			pthread_mutex_lock(   &(mt->lock) )
#define unlock_mem( mt )		pthread_mutex_unlock( &(mt->lock) )


struct lock_control
{
	pthread_mutex_t			hostalloc;					// mem hosts
	pthread_mutex_t			hashalloc;					// control of dhash allocation
	pthread_mutex_t			pointalloc;					// point list structures
	pthread_mutex_t			bufalloc;					// buffer structures
	pthread_mutex_t			listalloc;					// buflist structures

	pthread_mutex_t			hashstats;					// used for counters
	pthread_mutex_t			loop;						// thread startup/shutdown
	pthread_mutex_t			synth;						// synthetics control
	pthread_mutex_t			bufref;						// controlled buffer refcount

	pthread_mutex_t			table[HASHT_MUTEX_COUNT];	// hash table locks
	pthread_spinlock_t		dstats[DSTATS_SLOCK_COUNT];	// path stat data
	pthread_spinlock_t		dadder[DADDER_SLOCK_COUNT];	// path add data
	pthread_spinlock_t		dgauge[DGAUGE_SLOCK_COUNT];	// gauge add data

	int						init_done;
};



struct thread_data
{
	THRD				*	next;
	pthread_t				id;
	void				*	arg;
};



pthread_t thread_throw( void *(*fp) (void *), void *arg );

LOCK_CTL *lock_config_defaults( void );
void lock_shutdown( void );


#endif
