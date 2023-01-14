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
* strings/local.h - local structures and declarations for string handling *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef SHARED_STRINGUTILS_LOCAL_H
#define SHARED_STRINGUTILS_LOCAL_H



#define perm_lock( )		pthread_mutex_lock(   &(_str->perm_lock) )
#define perm_unlock( )		pthread_mutex_unlock( &(_str->perm_lock) )

#define stores_lock( )		pthread_mutex_lock(   &(_str->store_lock) )
#define stores_unlock( )	pthread_mutex_unlock( &(_str->store_lock) )

#define store_lock( _s )	pthread_mutex_lock(   &(_s->mtx) )
#define store_unlock( _s )	pthread_mutex_unlock( &(_s->mtx) )



#include "shared.h"


struct string_store
{
	SSTR				*	next;		// in case the caller wants a list
	SSTR				*	_str_next;	// a list to live in _str
	SSTE				**	hashtable;

	mem_free_cb			*	freefp;

	int64_t					hsz;
	int64_t					entries;
	int32_t					val_default;
	int32_t					set_default;
	int						freeable;

	pthread_mutex_t			mtx;
};


extern STR_CTL *_str;

#endif
