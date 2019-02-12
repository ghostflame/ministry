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

//	if( pthread_attr_setstacksize( tt_attr, _mem->stacksize << 10 ) )
//		err( "Cannot set default attr stacksize to %dKB -- %s",
//			_mem->stacksize, Err );

	tt_high = (pthread_attr_t *) allocz( sizeof( pthread_attr_t ) );
	pthread_attr_init( tt_high );

	if( pthread_attr_setdetachstate( tt_high, PTHREAD_CREATE_DETACHED ) )
		err( "Cannot set high-stack attr state to detached -- %s", Err );

	if( pthread_attr_setstacksize( tt_high, _mem->stackhigh << 10 ) )
		err( "Cannot set high-stack attr stacksize to %dKB -- %s",
			_mem->stackhigh, Err );
}


// wraps all thrown threads
void *__thread_wrapper( void *arg )
{
	THRD *t = (THRD *) arg;

	(*(t->call))( arg );
	free( t );

	pthread_exit( NULL );
	return NULL;
}


pthread_t __thread_throw( throw_fn *call, void *arg, pthread_attr_t **tt_a, int64_t num )
{
	THRD *t;

	if( *tt_a )
		thread_throw_init_attr( );

	t = (THRD *) allocz( sizeof( THRD ) );
	t->arg  = arg;
	t->num  = num;
	t->call = call;

	pthread_create( &(t->id), *tt_a, __thread_wrapper, t );

	return t->id;
}


pthread_t thread_throw( throw_fn *fp, void *arg, int64_t num )
{
	return __thread_throw( fp, arg, &tt_attr, num );
}

pthread_t thread_throw_high_stack( throw_fn *fp, void *arg, int64_t num )
{
	return __thread_throw( fp, arg, &tt_high, num );
}


pthread_t thread_throw_named( throw_fn *fp, void *arg, int64_t num, char *name )
{
	pthread_t tid;
	char buf[16];
	int l;

	tid = __thread_throw( fp, arg, &tt_attr, num );

	if( !name )
		return tid;

	memset( buf, 0, 16 );

	l = strlen( name );
	if( l > 15 )
		l = 15;

	memcpy( buf, name, l );

	// complain if it fails
	if( pthread_setname_np( tid, (const char *) buf ) )
		warn( "Could not set thread name to '%s' -- %s.", buf, Err );

	return tid;
}

pthread_t thread_throw_named_i( throw_fn *fp, void *arg, int64_t num, char *name )
{
	char fbuf[12], fmt[16], buf[16];
	int l;

	memset( fbuf, 0, 12 );
	memset( buf,  0, 16 );
	memset( fmt,  0, 16 );

	l = strlen( name );
	if( l > 11 )
		l = 11;

	memcpy( fbuf, name, l );

	snprintf( fmt, 16, "%s_%%ld", fbuf );
	snprintf( buf, 16, fmt, num );

	return thread_throw_named( fp, arg, num, buf );
}


