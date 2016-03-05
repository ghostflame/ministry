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
	debug( "Some %s loop thread has started.", tag );
	pthread_mutex_unlock( &(ctl->locks->loop) );
}

void loop_mark_done( const char *tag )
{
	pthread_mutex_lock( &(ctl->locks->loop) );
	ctl->loop_count--;
	debug( "Some %s loop thread has ended.", tag );
	pthread_mutex_unlock( &(ctl->locks->loop) );
}



// every tick, set the time
void loop_set_time( int64_t tval, void *arg )
{
	llts( tval, ctl->curr_time );
}


// we do integer maths here to avoid creeping
// double-precision addition inaccuracies
void loop_control( const char *name, loop_call_fn *fp, void *arg, int usec, int doSync, int offset )
{
	int64_t timer, nsec, noff, diff, t;
	struct timespec spec, now;

	// convert to nanoseconds to support nanosleep
	nsec = 1000 * (int64_t) usec;
	noff = 1000 * (int64_t) offset;

	clock_gettime( CLOCK_REALTIME, &now );
	timer = tsll( now );

	// do we synchronise to a clock?
	if( doSync )
	{
		t     = timer + noff + nsec - ( timer % nsec );
		diff  = t - timer;
		timer = t;

		llts( diff, spec );

		nanosleep( &spec, NULL );

		debug( "Pushed paper for %d usec to synchronize %s loop.", diff / 1000, name );
	}

	// say a loop has started
	loop_mark_start( name );

	while( ctl->run_flags & RUN_LOOP )
	{
		// call the payload
		(*fp)( timer, arg );

		// advance the clock
		timer += nsec;

		// get the current time
		clock_gettime( CLOCK_REALTIME, &now );

		// calculate how long to sleep for
		diff = timer - tsll( now );

		if( diff > 0 )
		{
			llts( diff, spec );

			// and sleep a bit
			nanosleep( &spec, NULL );
		}
	}

	// and say it's finished
	loop_mark_done( name );
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
	data_start( ctl->net->stats );
	data_start( ctl->net->adder );
	data_start( ctl->net->gauge );
	data_start( ctl->net->compat );

	// this just gather sets the time and doesn't exit
	loop_control( "time set", &loop_set_time, NULL, ctl->tick_usec, 1, 0 );
}


