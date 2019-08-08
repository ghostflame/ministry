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

#ifdef LOCK_DHASH_SPIN

typedef pthread_spinlock_t		dhash_lock_t;
#define lock_dhash( d )			pthread_spin_lock( d->lock )
#define unlock_dhash( d )		pthread_spin_unlock( d->lock )
#define linit_dhash( d )		pthread_spin_init( d->lock, PTHREAD_PROCESS_PRIVATE )

#else

typedef pthread_mutex_t			dhash_lock_t;
#define lock_dhash( d )			pthread_mutex_lock( d->lock )
#define unlock_dhash( d )		pthread_mutex_unlock( d->lock )
#define linit_dhash( d )		pthread_mutex_init( d->lock, &(ctl->proc->mtxa) )

#endif

#define lock_adder( d )			lock_dhash( d )
#define lock_stats( d )			lock_dhash( d )
#define lock_gauge( d )			lock_dhash( d )

#define unlock_adder( d )		unlock_dhash( d )
#define unlock_stats( d )		unlock_dhash( d )
#define unlock_gauge( d )		unlock_dhash( d )

#define lock_table( idx )		pthread_mutex_lock(   &(ctl->locks->table[idx & HASHT_MUTEX_MASK]) )
#define unlock_table( idx )		pthread_mutex_unlock( &(ctl->locks->table[idx & HASHT_MUTEX_MASK]) )

#define lock_tgtio( t )			pthread_mutex_lock(   &(t->lock) )
#define unlock_tgtio( t )		pthread_mutex_unlock( &(t->lock) )

#define lock_stthr( s )			pthread_mutex_lock(   &(s->lock) )
#define unlock_stthr( s )		pthread_mutex_unlock( &(s->lock) )

#define lock_synth( )			pthread_mutex_lock(   &(ctl->locks->synth) )
#define unlock_synth( )			pthread_mutex_unlock( &(ctl->locks->synth) )





struct lock_control
{
	pthread_mutex_t			hashstats;					// used for counters
	pthread_mutex_t			synth;						// synthetics control
	pthread_mutex_t			bufref;						// controlled buffer refcount

	pthread_mutex_t			table[HASHT_MUTEX_COUNT];	// hash table locks

	int						init_done;
};



LOCK_CTL *lock_config_defaults( void );
void lock_shutdown( void );


#endif
