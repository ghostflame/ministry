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
void loop_set_time( uint64_t tval, void *arg )
{
	ctl->curr_time.tv_sec  = tval / 1000000;
	ctl->curr_time.tv_usec = tval % 1000000;
}


// we do integer maths here to avoid creeping
// double-precision addition inaccuracies
void loop_control( const char *name, loop_call_fn *fp, void *arg, int usec, int doSync, int offset )
{
	uint64_t timer, next;
	struct timeval now;
	int sleepfor;

	gettimeofday( &now, NULL );
	timer = tvll( now );

	// do we synchronise to a clock?
	if( doSync )
	{
		next     = timer + offset + usec - ( timer % usec );
		sleepfor = (int) ( next - timer );
		timer    = next;

		usleep( sleepfor );
		debug( "Pushed paper for %d usec to synchronize %s loop.", sleepfor, name );
	}

	// say a loop has started
	loop_mark_start( name );

	while( ctl->run_flags & RUN_LOOP )
	{
		// call the payload
		(*fp)( timer, arg );

		// advance the clock
		timer += usec;

		// get the current time
		gettimeofday( &now, NULL );

		// calculate how long to sleep for
		sleepfor = timer - tvll( now );

		// and sleep a bit
		usleep( sleepfor );
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
	data_start( ctl->net->data );
	data_start( ctl->net->statsd );
	data_start( ctl->net->adder );

	// this just gather sets the time and doesn't exit
	loop_control( "time set", &loop_set_time, NULL, ctl->tick_usec, 1, 0 );
}


