/**************************************************************************
* Copyright 2015 John Denholm                                             *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
*                                                                         *
*                                                                         *
* thread.c - thread spawning and mutex control                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"


// wraps all thrown threads
void *__thread_wrapper( void *arg )
{
	THRD *t = (THRD *) arg;

	pthread_detach( t->id );

	if( t->name )
	{
		// complain if it fails
		if( pthread_setname_np( t->id, t->name ) )
			warn( "Could not set thread name to '%s' -- %s.", t->name, Err );
	}

	(*(t->call))( t );

	if( t->name )
		debug( "Thread has ended: %s", t->name );

	// minor risk of use-after-free - the caller
	// returns t->id.  Wrap in a sleep loop until
	// the caller sets a value inside t?  This is
	// after the payload, after all.
	free( t );

	pthread_exit( NULL );
	return NULL;
}


pthread_t __thread_throw( throw_fn *call, void *arg, int64_t num, char *name )
{
	THRD *t;
	int l;

	t = (THRD *) allocz( sizeof( THRD ) );
	t->arg  = arg;
	t->num  = num;
	t->call = call;

	if( name ) {
	  	l = strlen( name );
		if( l > 15 )
			l = 15;

		memcpy( (void *) t->name, name, l );
	}

	pthread_create( &(t->id), NULL, __thread_wrapper, t );

	return t->id;
}


pthread_t thread_throw( throw_fn *fp, void *arg, int64_t num )
{
	return __thread_throw( fp, arg, num, NULL );
}


pthread_t thread_throw_named( throw_fn *fp, void *arg, int64_t num, char *name )
{
	return __thread_throw( fp, arg, num, name );
}

pthread_t thread_throw_named_f( throw_fn *fp, void *arg, int64_t num, char *fmt, ... )
{
	char buf[16];
	va_list args;

	memset( buf, 0, 16 );
	va_start( args, fmt );
	vsnprintf( buf, 16, fmt, args );
	va_end( args );

	return __thread_throw( fp, arg, num, buf );
}


