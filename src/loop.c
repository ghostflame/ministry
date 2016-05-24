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
	debug( "Some %s loop thread has ended.  [%ld/%ld]", tag, skips, fires );
}



// every tick, set the time
void loop_set_time( int64_t tval, void *arg )
{
	llts( tval, ctl->curr_time );
}



static int64_t loop_control_factors[8] = {
	2, 3, 5, 7, 11, 13, 17, 19
};




// a simple timer loop
int64_t loop_simple( loop_call_fn *fp, void *arg, int64_t base, int64_t intv, int64_t *skips )
{
	struct timespec spec, now;
	int64_t fires, diff, t;

	fires  = 0;
	*skips = 0;

	while( ctl->run_flags & RUN_LOOP )
	{
		// call out to the payload
		(*fp)( base, arg );
		fires++;

		// advance the timer
		base += intv;

		// get the current time
		clock_gettime( CLOCK_REALTIME, &now );
		t = tsll( now );

		// just in case we are still behind do
		// this check, and count skips
		while( base <= t )
		{
			base += intv;
			(*skips)++;
		}

		// calculate how long to sleep for
		diff = base - t;
		llts( diff, spec );

		// and sleep a bit
		nanosleep( &spec, NULL );
	}

	return fires;
}

// a loop that fires more often to account for
// long loop times, but provides rapid response
int64_t loop_checks( const char *name, loop_call_fn *fp, void *arg, int64_t base, int64_t intv, int64_t *skips, int freq )
{
	struct timespec spec, now;
	int64_t fires, diff, t;
	int curr;

	fires  = 0;
	*skips = 0;
	curr   = 0;

	while( ctl->run_flags & RUN_LOOP )
	{
		if( ++curr == freq )
		{
			curr = 0;
			fires++;

			// call out to the payload
			(*fp)( base, arg );
		}

		// advance the timer
		base += intv;

		// get the current time
		clock_gettime( CLOCK_REALTIME, &now );
		t = tsll( now );

		// just in case we are still behind do
		// this check, and count skips
		while( base <= t )
		{
			base += intv;
			(*skips)++;
		}

		// calculate how long to sleep for
		diff = base - t;
		llts( diff, spec );

		// and sleep a bit
		nanosleep( &spec, NULL );
	}

	return fires;
}





// we do integer maths here to avoid creeping
// double-precision addition inaccuracies
void loop_control( const char *name, loop_call_fn *fp, void *arg, int usec, int flags, int offset )
{
	int64_t diff, t, nsec, intv, offs, timer, skips, fires;
	struct timespec spec;
	int i, max;

	// convert to nsec
	nsec = 1000 * (int64_t) usec;
	offs = 1000 * (int64_t) offset;

	// the actual sleep interval may be less
	// if nsec is too high
	intv = nsec;
	max  = 1;

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
		intv /= loop_control_factors[i];
		max  *= loop_control_factors[i];
	}

	if( max > 1 )
		debug( "Loop %s trimmed from %ld to %ld nsec interval.", name, nsec, intv );

	// get the time
	clock_gettime( CLOCK_REALTIME, &spec );
	timer = tsll( spec );

	// do we synchronise to a clock?
	if( flags & LOOP_SYNC )
	{
		t     = timer + offs + nsec - ( timer % nsec );
		diff  = t - timer;
		timer = t;

		llts( diff, spec );
		nanosleep( &spec, NULL );

		debug( "Pushed paper for %d usec to synchronize %s loop.", diff / 1000, name );
	}

	skips = 0;

	// say a loop has started
	loop_mark_start( name );

	// call the appropriate loop fn
	if( max > 0 )
		fires = loop_checks( name, fp, arg, timer, intv, &skips, max );
	else
		fires = loop_simple( fp, arg, timer, intv, &skips );

	// and say it's finished
	loop_mark_done( name, skips, fires );
}



void loop_start( void )
{
	// set us going
	ctl->run_flags |= RUN_LOOP;

	get_time( );

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

	// and the network io loops
	io_start( );

	// throw the data listener loop
	net_start_type( ctl->net->stats );
	net_start_type( ctl->net->adder );
	net_start_type( ctl->net->gauge );
	net_start_type( ctl->net->compat );

	// this just gather sets the time and doesn't exit
	loop_control( "time set", &loop_set_time, NULL, ctl->tick_usec, LOOP_SYNC|LOOP_TRIM, 0 );
}


