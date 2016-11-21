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



#ifdef KEEP_LOCK_STATS

#define lock_adder( d )			pthread_spin_lock( &(ctl->locks->dadder->locks[d->id & DADDER_SLOCK_MASK]) ); \
								ctl->locks->dadder->used[d->id & DADDER_SLOCK_MASK].count++

#define lock_stats( d )			pthread_spin_lock( &(ctl->locks->dstats->locks[d->id & DSTATS_SLOCK_MASK]) ); \
								ctl->locks->dstats->used[d->id & DSTATS_SLOCK_MASK].count++

#define lock_gauge( d )			pthread_spin_lock( &(ctl->locks->dgauge->locks[d->id & DGAUGE_SLOCK_MASK]) ); \
								ctl->locks->dgauge->used[d->id & DGAUGE_SLOCK_MASK].count++

#else

#define lock_adder( d )			pthread_spin_lock( &(ctl->locks->dadder->locks[d->id & DADDER_SLOCK_MASK]) )
#define lock_stats( d )			pthread_spin_lock( &(ctl->locks->dstats->locks[d->id & DSTATS_SLOCK_MASK]) )
#define lock_gauge( d )			pthread_spin_lock( &(ctl->locks->dgauge->locks[d->id & DGAUGE_SLOCK_MASK]) )

#endif

#define unlock_stats( d )		pthread_spin_unlock( &(ctl->locks->dstats->locks[d->id & DSTATS_SLOCK_MASK]) )
#define unlock_adder( d )		pthread_spin_unlock( &(ctl->locks->dadder->locks[d->id & DADDER_SLOCK_MASK]) )
#define unlock_gauge( d )		pthread_spin_unlock( &(ctl->locks->dgauge->locks[d->id & DGAUGE_SLOCK_MASK]) )

#define lock_table( idx )		pthread_mutex_lock(   &(ctl->locks->table[idx & HASHT_MUTEX_MASK]) )
#define unlock_table( idx )		pthread_mutex_unlock( &(ctl->locks->table[idx & HASHT_MUTEX_MASK]) )

#define lock_tgtio( t )			pthread_mutex_lock(   &(t->lock) )
#define unlock_tgtio( t )		pthread_mutex_unlock( &(t->lock) )

#define lock_stthr( s )			pthread_mutex_lock(   &(s->lock) )
#define unlock_stthr( s )		pthread_mutex_unlock( &(s->lock) )

#define lock_synth( )			pthread_mutex_lock(   &(ctl->locks->synth) )
#define unlock_synth( )			pthread_mutex_unlock( &(ctl->locks->synth) )

#define lock_mem( mt )			pthread_mutex_lock(   &(mt->lock) )
#define unlock_mem( mt )		pthread_mutex_unlock( &(mt->lock) )

#define lock_ntype( nt )		pthread_mutex_lock(   &(nt->lock) )
#define unlock_ntype( nt )		pthread_mutex_unlock( &(nt->lock) )

#define lock_ldap( s )			pthread_mutex_lock(   &(s->lock) )
#define unlock_ldap( s )		pthread_mutex_unlock( &(s->lock) )

#define lock_tokens( )			pthread_mutex_lock(   &(ctl->locks->tokens) )
#define unlock_tokens( )		pthread_mutex_unlock( &(ctl->locks->tokens) )


struct dhash_locks
{
	pthread_spinlock_t	*	locks;
	LLCT				*	used;
	char				*	name;
	uint32_t				len;
	uint32_t				mask;
};


struct lock_control
{
	pthread_mutex_t			tokens;						// token control
	pthread_mutex_t			hashstats;					// used for counters
	pthread_mutex_t			loop;						// thread startup/shutdown
	pthread_mutex_t			synth;						// synthetics control
	pthread_mutex_t			bufref;						// controlled buffer refcount

	pthread_mutex_t			table[HASHT_MUTEX_COUNT];	// hash table locks

	DLOCKS				*	dadder;
	DLOCKS				*	dstats;
	DLOCKS				*	dgauge;

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
