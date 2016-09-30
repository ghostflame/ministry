/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* loop.c - run control / loop control functions                           *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "ministry.h"

void loop_end( char *reason )
{
	info( "Shutting down polling: %s", reason );
	ctl->run_flags |=  RUN_SHUTDOWN;
	ctl->run_flags &= ~RUN_LOOP;
}

void loop_kill( int sig )
{
	notice( "Received signal %d", sig );
	ctl->run_flags |=  RUN_SHUTDOWN;
	ctl->run_flags &= ~RUN_LOOP;
}


void loop_mark_start( const char *tag )
{
	pthread_mutex_lock( &(ctl->locks->loop) );
	ctl->loop_count++;
	pthread_mutex_unlock( &(ctl->locks->loop) );
	debug( "Some %s loop thread has started.", tag );
}

void loop_mark_done( const char *tag, int64_t skips, int64_t fires )
{
	pthread_mutex_lock( &(ctl->locks->loop) );
	ctl->loop_count--;
	pthread_mutex_unlock( &(ctl->locks->loop) );
	debug( "Some %s loop thread has ended.  [%ld/%ld]", tag, fires, skips );
}



// every tick, set the time
void loop_set_time( int64_t tval, void *arg )
{
	llts( tval, ctl->curr_time );
}


void *loop_timer( void *arg )
{
	THRD *t = (THRD *) arg;

	// this just gather sets the time and doesn't exit
	loop_control( "time set", &loop_set_time, NULL, ctl->tick_usec, LOOP_SYNC|LOOP_SILENT, 0 );

	free( t );
	return NULL;
}






static int64_t loop_control_factors[8] = {
	2, 3, 5, 7, 11, 13, 17, 19
};



// we do integer maths here to avoid creeping
// double-precision addition inaccuracies
void loop_control( const char *name, loop_call_fn *fp, void *arg, int usec, int flags, int offset )
{
	int64_t timer, intv, nsec, offs, diff, t, skips, fires;
	int i, ticks = 1, curr = 0;
	struct timespec ts;
#ifdef DEBUG_LOOPS
	int64_t marker = 1;
#endif

	// convert to nsec
	nsec = 1000 * (int64_t) usec;
	offs = 1000 * (int64_t) offset;
	// the actual sleep interval may be less
	// if period is too high
	intv = nsec;

	// wind down to an acceptable interval
	// try to avoid issues
	while( ( flags & LOOP_TRIM ) && intv > MAX_LOOP_NSEC && intv > offs )
	{
		for( i = 0; i < 8; i++ )
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
		debug( "Loop %s trimmed from %ld to %ld nsec interval.", name, nsec, intv );

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

	while( ctl->run_flags & RUN_LOOP )
	{
		// decide if we are firing the payload
		if( ++curr == ticks )
		{
#ifdef DEBUG_LOOPS
			if( !( flags & LOOP_SILENT ) )
				debug_loop( "Calling payload %s", name );
#endif
			(*fp)( timer, arg );
			fires++;
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
			skips++;
#ifdef DEBUG_LOOPS
			if( skips == marker )
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



void loop_start( void )
{
	// set us going
	ctl->run_flags |= RUN_LOOP;

	get_time( );

	// start a timing circuit
	thread_throw( &loop_timer, NULL );

	// and the tset/target loops
	// must happen before stats_start
	target_start( );

	// see if we need ldap
	auth_ldap_start( );

	// throw the data submission loops
	stats_start( ctl->stats->stats );
	stats_start( ctl->stats->adder );
	stats_start( ctl->stats->gauge );
	stats_start( ctl->stats->self );

	// and a synthetics loop
	thread_throw( &synth_loop, NULL );

	// throw the memory control loop
	thread_throw( &mem_loop, NULL );

	// and gc
	thread_throw( &gc_loop, NULL );

	// throw the data listener loop
	net_start_type( ctl->net->stats );
	net_start_type( ctl->net->adder );
	net_start_type( ctl->net->gauge );
	net_start_type( ctl->net->compat );

	// and now we wait for the signal to end
	while( ctl->run_flags & RUN_LOOP )
		sleep( 1 );
}


