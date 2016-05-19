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


DLOCKS *dhash_locks_create( int type )
{
	const DTYPE *dt = data_type_defns + type;
	DLOCKS *d;
	int i;

	d = (DLOCKS *) allocz( sizeof( DLOCKS ) );

	d->len   = dt->lock;
	d->mask  = d->len - 1;
	d->name  = dt->name;
	d->used  = (LLCT *) allocz( d->len * sizeof( LLCT ) );
	d->locks = (pthread_spinlock_t *) allocz( d->len * sizeof( pthread_spinlock_t ) );

	// and init the locks
	for( i = 0; i < d->len; i++ )
		pthread_spin_init( d->locks + i, PTHREAD_PROCESS_PRIVATE );

	return d;
}

void dhash_locks_destroy( DLOCKS *d )
{
	int i;

	for( i = 0; i < d->len; i++ )
		pthread_spin_destroy( d->locks + i );
}


LOCK_CTL *lock_config_defaults( void )
{
	LOCK_CTL *l;
	int i;

	l = (LOCK_CTL *) allocz( sizeof( LOCK_CTL ) );

	// make the stats structures
	l->dadder = dhash_locks_create( DATA_TYPE_ADDER );
	l->dstats = dhash_locks_create( DATA_TYPE_STATS );
	l->dgauge = dhash_locks_create( DATA_TYPE_GAUGE );

	// and init all the mutexes

	// used in mem.c
	pthread_mutex_init( &(l->hostalloc), NULL );
	pthread_mutex_init( &(l->hashalloc), NULL );
	pthread_mutex_init( &(l->pointalloc), NULL );
	pthread_mutex_init( &(l->bufalloc), NULL );
	pthread_mutex_init( &(l->listalloc), NULL );

	// used to keep counters
	pthread_mutex_init( &(l->hashstats), NULL );

	// used in loop control
	pthread_mutex_init( &(l->loop), NULL );

	// used in synths/stats
	pthread_mutex_init( &(l->synth), NULL );

	// used to control io buffer expiry
	pthread_mutex_init( &(l->bufref), NULL );

	// used to lock table positions
	for( i = 0; i < HASHT_MUTEX_COUNT; i++ )
		pthread_mutex_init( l->table + i, NULL );

	l->init_done = 1;
	return l;
}

void lock_shutdown( void )
{
	LOCK_CTL *l = ctl->locks;
	ST_THR *t;
	int i;

	if( !l || !l->init_done )
		return;

	// used in mem.c
	pthread_mutex_destroy( &(l->hostalloc) );
	pthread_mutex_destroy( &(l->hashalloc) );
	pthread_mutex_destroy( &(l->pointalloc) );
	pthread_mutex_destroy( &(l->bufalloc) );
	pthread_mutex_destroy( &(l->listalloc) );

	// used to keep counters
	pthread_mutex_destroy( &(l->hashstats) );

	// used in loop control
	pthread_mutex_destroy( &(l->loop) );

	// used in synths/stats
	pthread_mutex_destroy( &(l->synth) );

	// used in buffer refs
	pthread_mutex_destroy( &(l->bufref) );

	// used to lock table positions
	for( i = 0; i < HASHT_MUTEX_COUNT; i++ )
		pthread_mutex_destroy( l->table + i );

	// used to lock paths
	dhash_locks_destroy( l->dstats );
	dhash_locks_destroy( l->dadder );
	dhash_locks_destroy( l->dgauge );

	// used in stats thread control
	for( t = ctl->stats->adder->ctls; t; t = t->next )
		pthread_mutex_destroy( &(t->lock) );

	l->init_done = 0;
}

