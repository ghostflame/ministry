/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* thread.c - thread spawning and mutex control                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry_test.h"


pthread_attr_t *tt_attr = NULL;

void thread_throw_init_attr( void )
{
	tt_attr = (pthread_attr_t *) allocz( sizeof( pthread_attr_t ) );
	pthread_attr_init( tt_attr );

	if( pthread_attr_setdetachstate( tt_attr, PTHREAD_CREATE_DETACHED ) )
		err( "Cannot set default attr state to detached -- %s", Err );

	if( pthread_attr_setstacksize( tt_attr, ctl->mem->stacksize << 10 ) )
		err( "Cannot set default attr stacksize to %dKB -- %s",
			ctl->mem->stacksize >> 10, Err );
}


pthread_t thread_throw( void *(*fp) (void *), void *arg, int64_t num )
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


