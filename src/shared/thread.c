/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* thread.c - thread spawning and mutex control                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"


pthread_attr_t *tt_attr = NULL;
pthread_attr_t *tt_high = NULL;

void thread_throw_init_attr( void )
{
	tt_attr = (pthread_attr_t *) allocz( sizeof( pthread_attr_t ) );
	pthread_attr_init( tt_attr );

	if( pthread_attr_setdetachstate( tt_attr, PTHREAD_CREATE_DETACHED ) )
		err( "Cannot set default attr state to detached -- %s", Err );

	if( pthread_attr_setstacksize( tt_attr, _mem->stacksize << 10 ) )
		err( "Cannot set default attr stacksize to %dKB -- %s",
			_mem->stacksize, Err );

	tt_high = (pthread_attr_t *) allocz( sizeof( pthread_attr_t ) );
	pthread_attr_init( tt_high );

	if( pthread_attr_setdetachstate( tt_high, PTHREAD_CREATE_DETACHED ) )
		err( "Cannot set high-stack attr state to detached -- %s", Err );

	if( pthread_attr_setstacksize( tt_high, _mem->stackhigh << 10 ) )
		err( "Cannot set high-stack attr stacksize to %dKB -- %s",
			_mem->stackhigh, Err );
}


pthread_t thread_throw( throw_fn *fp, void *arg, int64_t num )
{
	THRD *t;

	if( !tt_attr )
		thread_throw_init_attr( );

	t = (THRD *) allocz( sizeof( THRD ) );
	t->arg = arg;
	t->num = num;
	pthread_create( &(t->id), tt_attr, fp, t );

	return t->id;
}

pthread_t thread_throw_high( throw_fn *fp, void *arg, int64_t num )
{
	THRD *t;

	if( !tt_high )
		thread_throw_init_attr( );

	t = (THRD *) allocz( sizeof( THRD ) );
	t->arg = arg;
	t->num = num;
	pthread_create( &(t->id), tt_high, fp, t );

	return t->id;
}


pthread_t thread_throw_watched( throw_fn *watcher, throw_fn *fp, void *arg, int64_t num )
{
	THRD *t;

	if( !tt_attr )
		thread_throw_init_attr( );

	t = (THRD *) allocz( sizeof( THRD ) );
	t->arg = arg;
	t->num = num;
	t->fp  = fp;
	pthread_create( &(t->id), tt_attr, watcher, t );

	return t->id;
}

