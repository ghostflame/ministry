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
* thread.h - threading structures, mutexes and locking macros             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_THREAD_H
#define MINISTRY_THREAD_H

#define HASHT_MUTEX_COUNT		0x20	// 32
#define HASHT_MUTEX_MASK		0x1f

#ifdef LOCK_DHASH_SPIN

typedef pthread_spinlock_t		dhash_lock_t;
#define lock_dhash( d )			pthread_spin_lock( d->lock )
#define unlock_dhash( d )		pthread_spin_unlock( d->lock )
#define linit_dhash( d )		pthread_spin_init( d->lock, PTHREAD_PROCESS_PRIVATE )

#else

typedef pthread_mutex_t			dhash_lock_t;
#define lock_dhash( d )			pthread_mutex_lock( d->lock )
#define unlock_dhash( d )		pthread_mutex_unlock( d->lock )
#define linit_dhash( d )		pthread_mutex_init( d->lock, &(ctl->proc->mem->mtxa) )

#endif

#define lock_adder( d )			lock_dhash( d )
#define lock_stats( d )			lock_dhash( d )
#define lock_gauge( d )			lock_dhash( d )
#define lock_histo( d )			lock_dhash( d )

#define unlock_adder( d )		unlock_dhash( d )
#define unlock_stats( d )		unlock_dhash( d )
#define unlock_gauge( d )		unlock_dhash( d )
#define unlock_histo( d )		unlock_dhash( d )

#define lock_table( idx )		pthread_mutex_lock(   &(ctl->locks->table[idx & HASHT_MUTEX_MASK]) )
#define unlock_table( idx )		pthread_mutex_unlock( &(ctl->locks->table[idx & HASHT_MUTEX_MASK]) )

#define lock_tgtio( t )			pthread_mutex_lock(   &(t->lock) )
#define unlock_tgtio( t )		pthread_mutex_unlock( &(t->lock) )

#define lock_stthr( s )			pthread_mutex_lock(   &(s->lock) )
#define unlock_stthr( s )		pthread_mutex_unlock( &(s->lock) )

#define lock_synth( )			pthread_mutex_lock(   &(ctl->locks->synth) )
#define unlock_synth( )			pthread_mutex_unlock( &(ctl->locks->synth) )

#define lock_stat_cfg( _c )		pthread_mutex_lock(   &(_c->statslock ) )
#define unlock_stat_cfg( _c )	pthread_mutex_unlock( &(_c->statslock ) )




struct lock_control
{
	pthread_mutex_t			synth;						// synthetics control
	pthread_mutex_t			table[HASHT_MUTEX_COUNT];	// hash table locks

	int						init_done;
};



LOCK_CTL *lock_config_defaults( void );
void lock_shutdown( void );


#endif
