/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* thread.c - thread spawning and mutex control                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"


pthread_attr_t *tt_attr = NULL;

void thread_throw_init_attr( void )
{
	tt_attr = (pthread_attr_t *) allocz( sizeof( pthread_attr_t ) );
	pthread_attr_init( tt_attr );

	if( pthread_attr_setdetachstate( tt_attr, PTHREAD_CREATE_DETACHED ) )
		err( "Cannot set default attr state to detached -- %s", Err );
}


pthread_t thread_throw( void *(*fp) (void *), void *arg )
{
	THRD *t;

	if( !tt_attr )
		thread_throw_init_attr( );

	t = (THRD *) allocz( sizeof( THRD ) );
	t->arg = arg;
	pthread_create( &(t->id), tt_attr, fp, t );

	return t->id;
}


// throw a network socket watcher and a handler
void thread_throw_watched( void *(*fp) (void *), void *arg )
{
	THRD *t;

	if( !tt_attr )
		thread_throw_init_attr( );

	t = (THRD *) allocz( sizeof( THRD ) );
	t->arg = arg;
	t->fp  = fp;
	pthread_create( &(t->id), tt_attr, &net_watched_socket, t );
}


LOCK_CTL *lock_config_defaults( void )
{
	LOCK_CTL *l;
	int i;

	l = (LOCK_CTL *) allocz( sizeof( LOCK_CTL ) );

	// and init all the mutexes

	// used in mem.c
	pthread_mutex_init( &(l->hostalloc), NULL );
	pthread_mutex_init( &(l->hashalloc), NULL );
	pthread_mutex_init( &(l->pointalloc), NULL );
	pthread_mutex_init( &(l->bufalloc), NULL );

	// used to keep counters
	pthread_mutex_init( &(l->hashstats), NULL );

	// used in networking
	pthread_mutex_init( &(l->iobuffers), NULL );

	// used in loop control
	pthread_mutex_init( &(l->loop), NULL );

	// used to lock table positions
	for( i = 0; i < HASHT_MUTEX_COUNT; i++ )
		pthread_mutex_init( l->table + i, NULL );

	// used to lock paths
	for( i = 0; i < DSTATS_MUTEX_COUNT; i++ )
		pthread_mutex_init( l->dstats + i, NULL );
	for( i = 0; i < DADDER_MUTEX_COUNT; i++ )
		pthread_mutex_init( l->dadder + i, NULL );

	l->init_done = 1;
	return l;
}

void lock_shutdown( void )
{
	LOCK_CTL *l = ctl->locks;
	int i;

	if( !l || !l->init_done )
		return;

	// used in mem.c
	pthread_mutex_destroy( &(l->hostalloc) );
	pthread_mutex_destroy( &(l->hashalloc) );
	pthread_mutex_destroy( &(l->pointalloc) );
	pthread_mutex_destroy( &(l->bufalloc) );

	// used to keep counters
	pthread_mutex_destroy( &(l->hashstats) );

	// used in networking
	pthread_mutex_destroy( &(l->iobuffers) );

	// used in loop control
	pthread_mutex_destroy( &(l->loop) );

	// used to lock table positions
	for( i = 0; i < HASHT_MUTEX_COUNT; i++ )
		pthread_mutex_destroy( l->table + i );

	// used to lock paths
	for( i = 0; i < DSTATS_MUTEX_COUNT; i++ )
		pthread_mutex_destroy( l->dstats + i );
	for( i = 0; i < DADDER_MUTEX_COUNT; i++ )
		pthread_mutex_destroy( l->dadder + i );

	l->init_done = 0;
}

