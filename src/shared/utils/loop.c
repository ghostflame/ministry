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
* loop.c - run control / loop control functions                           *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "shared.h"

void loop_end( const char *reason )
{
	info( "Shutting down polling: %s", reason );
	runf_add( RUN_SHUTDOWN );
	runf_rmv( RUN_LOOP );
}

void loop_kill( int sig )
{
	notice( "Received signal %d", sig );
	runf_add( RUN_SHUTDOWN );
	runf_rmv( RUN_LOOP );
}


void loop_mark_start( const char *tag )
{
	pthread_mutex_lock( &(_proc->loop_lock) );
	++(_proc->loop_count);
	pthread_mutex_unlock( &(_proc->loop_lock) );
	if( tag )
		debug( "Some %s loop thread has started.", tag );
}

void loop_mark_done( const char *tag, int64_t skips, int64_t fires )
{
	pthread_mutex_lock( &(_proc->loop_lock) );
	_proc->loop_count--;
	pthread_mutex_unlock( &(_proc->loop_lock) );
	if( tag )
		debug( "Some %s loop thread has ended.  [%ld/%ld]", tag, fires, skips );
}



// every tick, set the time
void loop_set_time( int64_t tval, void *arg )
{
	llts( tval, _proc->curr_time );
	_proc->curr_tval = tval;
	_proc->curr_usec = tval / 1000;
}


void loop_timer( THRD *t )
{
	// this just gather sets the time and doesn't exit
	loop_control( "time set", &loop_set_time, NULL, _proc->tick_usec, LOOP_SYNC, 0 );
}






static int64_t loop_control_factors[8] = {
	2, 3, 5, 7, 11, 13, 17, 19
};



// we do integer maths here to avoid creeping
// double-precision addition inaccuracies
void loop_control( const char *name, loop_call_fn *fp, void *arg, int64_t usec, int flags, int64_t offset )
{
	int64_t timer, intv, nsec, offs, diff, t, skips, fires;
	int i, ticks = 1, curr = 0;
	struct timespec ts;
#ifdef DEBUG_LOOPS
	int64_t marker = 1;
#endif

	// convert to nsec
	nsec = 1000 * usec;
	offs = 1000 * offset;
	// the actual sleep interval may be less
	// if period is too high
	intv = nsec;

	// trim the offset to <= nsec
	if( offs >= nsec )
		offs = offs % nsec;

	// wind down to an acceptable interval
	// try to avoid issues
	while( ( flags & LOOP_TRIM ) && intv > MAX_LOOP_NSEC )
	{
		for( i = 0; i < 8; ++i )
			if( ( intv % loop_control_factors[i] ) == 0 )
				break;

		// if we can't find a suitable factor then try 2
		// this may introduce long-term instability :-(
		if( i == 8 )
		{
			warn( "Could not find a suitable prime factor for %s interval nsec.", name );
			i = 0;
		}

		// and adjust
		intv  /= loop_control_factors[i];
		ticks *= loop_control_factors[i];
	}

	if( ticks > 1 )
	{
		debug( "Loop %s trimmed from %ld to %ld nsec interval.", name, nsec, intv );
		// needed to avoid calling it too quickly first time,
		// which breaks offsets
		curr = -1;
	}

	// get the time
	clock_gettime( CLOCK_REALTIME, &ts );
	timer = tsll( ts );

	// do we synchronise to a clock?
	if( flags & LOOP_SYNC )
	{
		t     = timer + offs + nsec - ( timer % nsec );
		diff  = t - timer;
		timer = t;

		llts( diff, ts );
		nanosleep( &ts, NULL );

		debug( "Pushed paper for %d usec to synchronize %s loop.", diff / 1000, name );
	}

	fires = 0;
	skips = 0;

	// say a loop has started
	loop_mark_start( name );

	while( RUNNING( ) )
	{
#ifdef DEBUG_LOOPS
		if( flags & LOOP_DEBUG )
		{
			clock_gettime( CLOCK_REALTIME, &ts );
			t = tsll( ts );
			debug( "Timer for %s now %ld, curr = %d, time = %ld", name, timer / 1000, curr, t / 1000 );
		}
#endif

		// decide if we are firing the payload
		if( ++curr == ticks )
		{
#ifdef DEBUG_LOOPS
			if( flags & LOOP_DEBUG )
				debug( "Calling payload %s", name );
#endif
			(*fp)( timer, arg );
			++fires;
			curr = 0;
		}

		// roll on the timer
		timer += intv;


		// get the current time
		clock_gettime( CLOCK_REALTIME, &ts );
		t = tsll( ts );

		// don't do negative sleep
		if( t < timer )
		{
			// and sleep
			diff = timer - t;
			llts( diff, ts );
			nanosleep( &ts, NULL );
		}
		else
		{
			++skips;
#ifdef DEBUG_LOOPS
			if( flags & LOOP_DEBUG && skips == marker )
			{
				debug( "Loop %s skips: %ld", name, skips );
				marker = marker << 1;
			}
#endif
		}
	}

	// and say it's finished
	loop_mark_done( name, skips, fires );
}


