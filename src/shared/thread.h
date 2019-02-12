/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* thread.h - threading structures and functions                           *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_THREAD_H
#define SHARED_THREAD_H


struct thread_data
{
	THRD				*	next;
	pthread_t				id;
	void				*	arg;
	throw_fn			*	call;
	int64_t					num;
};

pthread_t thread_throw( throw_fn *fp, void *arg, int64_t num );
pthread_t thread_throw_named( throw_fn *fp, void *arg, int64_t num, char *name );
pthread_t thread_throw_named_i( throw_fn *fp, void *arg, int64_t num, char *name );
pthread_t thread_throw_high_stack( throw_fn *fp, void *arg, int64_t num );


#endif
