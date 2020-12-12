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
	const char				name[16];
};

pthread_t thread_throw( throw_fn *fp, void *arg, int64_t num );
pthread_t thread_throw_named( throw_fn *fp, void *arg, int64_t num, char *name );
pthread_t thread_throw_named_f( throw_fn *fp, void *arg, int64_t num, char *fmt, ... );


#endif
