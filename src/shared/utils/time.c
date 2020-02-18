/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* utils/time.c - time utilities                                           *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"



void get_time( void )
{
	if( _proc )
	{
		clock_gettime( CLOCK_REALTIME, &(_proc->curr_time) );
		_proc->curr_tval = tsll( _proc->curr_time );
		_proc->curr_usec = _proc->curr_tval / 1000;
	}
}

int64_t get_time64( void )
{
	struct timespec ts;

	clock_gettime( CLOCK_REALTIME, &ts );
	return tsll( ts );
}



double ts_diff( struct timespec to, struct timespec from, double *store )
{
	double diff;

	diff  = (double) ( to.tv_nsec - from.tv_nsec );
	diff /= 1000000000.0;
	diff += (double) ( to.tv_sec  - from.tv_sec  );

	if( store )
		*store = diff;

	return diff;
}


double get_uptime( void )
{
	double diff;

	get_time( );
	ts_diff( _proc->curr_time, _proc->init_time, &diff );

	return diff;
}

time_t get_uptime_sec( void )
{
	get_time( );
	return _proc->curr_time.tv_sec - _proc->init_time.tv_sec;
}


void microsleep( int64_t usec )
{
	struct timespec slpr;

	usec *= 1000;
	llts( usec, slpr );

	nanosleep( &slpr, NULL );
}

