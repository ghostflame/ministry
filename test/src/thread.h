/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* thread.h - threading structures, mutexes and locking macros             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_TEST_THREAD_H
#define MINISTRY_TEST_THREAD_H


#define lock_iolist( l )		pthread_mutex_lock(   &(l->lock) )
#define unlock_iolist( l )		pthread_mutex_unlock( &(l->lock) )

#define lock_mem( mt )			pthread_mutex_lock(   &(mt->lock) )
#define unlock_mem( mt )		pthread_mutex_unlock( &(mt->lock) )


struct lock_control
{
	pthread_mutex_t			loop;						// thread startup/shutdown

	int						init_done;
};



struct thread_data
{
	THRD				*	next;
	pthread_t				id;
	void				*	arg;
	int64_t					num;
};



pthread_t thread_throw( void *(*fp) (void *), void *arg, int64_t num );

LOCK_CTL *lock_config_defaults( void );
void lock_shutdown( void );


#endif
