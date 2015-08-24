#ifndef MINISTRY_THREAD_H
#define MINISTRY_THREAD_H


#define DSTAT_MUTEX_COUNT		0x40	// 64
#define DSTAT_MUTEX_MASK		0x3f

#define DADD_MUTEX_COUNT		0x40	// 64
#define DADD_MUTEX_MASK			0x3f


#define lock_adder( d )			pthread_mutex_lock(   &(ctl->locks->dadd[d->id & DADD_MUTEX_MASK]) )
#define unlock_adder( d )		pthread_mutex_unlock( &(ctl->locks->dadd[d->id & DADD_MUTEX_MASK]) )

#define lock_stats( d )			pthread_mutex_lock(   &(ctl->locks->dstat[d->id & DSTAT_MUTEX_MASK]) )
#define unlock_stats( d )		pthread_mutex_unlock( &(ctl->locks->dstat[d->id & DSTAT_MUTEX_MASK]) )



struct lock_control
{
	pthread_mutex_t			hostalloc;					// mem hosts
	pthread_mutex_t			statalloc;					// control of dstat allocation
	pthread_mutex_t			addalloc;					// control of dadd allocation
	pthread_mutex_t			pointalloc;					// point list structures

	pthread_mutex_t			loop;						// thread startup/shutdown
	pthread_mutex_t			stattable;					// insertion into stats hash
	pthread_mutex_t			addtable;					// insertion into dd hash

	pthread_mutex_t			dstat[DSTAT_MUTEX_COUNT];	// path stat data
	pthread_mutex_t			dadd[DADD_MUTEX_COUNT];		// path add data

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
