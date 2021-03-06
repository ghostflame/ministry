/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* thread.c - thread spawning and mutex control                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"


LOCK_CTL *lock_config_defaults( void )
{
	LOCK_CTL *l;
	int i;

	l = (LOCK_CTL *) mem_perm( sizeof( LOCK_CTL ) );

	// and init all the mutexes

	// used in synths/stats
	pthread_mutex_init( &(l->synth), &(ctl->proc->mem->mtxa) );

	// used to lock table positions
	for( i = 0; i < HASHT_MUTEX_COUNT; ++i )
		pthread_mutex_init( l->table + i, &(ctl->proc->mem->mtxa) );

	l->init_done = 1;
	return l;
}

void lock_shutdown( void )
{
	LOCK_CTL *l = ctl->locks;
	int i;

	if( !l || !l->init_done )
		return;

	// used in synths/stats
	pthread_mutex_destroy( &(l->synth) );

	// used to lock table positions
	for( i = 0; i < HASHT_MUTEX_COUNT; ++i )
		pthread_mutex_destroy( l->table + i );

	l->init_done = 0;
}

