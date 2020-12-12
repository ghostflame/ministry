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

