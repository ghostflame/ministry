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

// trimmed to the nearest msec
int get_uptime_msec( char *buf, int len )
{
	snprintf( buf, len, "%.3f", get_uptime( ) );
	return len;
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

int time_span_usec( const char *str, int64_t *usec )
{
	int64_t v;
	char *u;

	*usec = 0;

	v = strtoll( str, &u, 10 );

	if( !v )
		return -1;

	if( u != NULL )
	{
		// order matters, because of the reuse of 'm'
		if( !strprefix( u, "years" ) || !strprefix( u, "yrs" ) )
		{
			v *= 365 * 24 * 60 * 60 * MILLION;
//			info( "Recognised %s as years.", str );
		}
		else if( !strprefix( u, "weeks" ) || !strprefix( u, "wks" ) )
		{
			v *= 7 * 24 * 60 * 60 * MILLION;
//			info( "Recognised %s as weeks.", str );
		}
		else if( !strprefix( u, "days" ) )
		{
			v *= 24 * 60 * 60 * MILLION;
//			info( "Recognised %s as days.", str );
		}
		else if( !strprefix( u, "hours" ) || !strprefix( u, "hrs" ) )
		{
			v *= 60 * 60 * MILLION;
//			info( "Recognised %s as hours.", str );
		}
		else if( !strprefix( u, "minutes" ) || !strprefix( u, "mins" ) )
		{
			v *= 60 * MILLION;
//			info( "Recognised %s as minutes.", str );
		}
		else if( !strprefix( u, "seconds" ) || !strprefix( u, "secs" ) )
		{
			v *= MILLION;
//			info( "Recognised %s as seconds.", str );
		}
		else if( !strprefix( u, "msecs" ) || !strprefix( u, "milliseconds" ) )
		{
			v *= 1000;
//			info( "Recognised %s as msec.", str );
		}
		else if( !strprefix( u, "months" ) || !strprefix( u, "mths" ) )
		{
			v *= 30 * 24 * 60 * 60 * MILLION;
//			info( "Recognised %s as months.", str );
		}
		else if( !strprefix( u, "usecs" ) || !strprefix( u, "microseconds" ) )
		{
			v *= 1;
//			info( "Recognised %s as usec.", str );
		}
		else
		{
			err( "Time suffix '%s' not recognised.", u );
			return -1;
		}
	}

	*usec = v;
	return 0;
}


#define NSEC_THRESH		250 * MILLION * BILLION
#define USEC_THRESH		250 * MILLION * MILLION
#define MSEC_THRESH		250 * BILLION

int time_usec( const char *str, int64_t *usec )
{
	int64_t v;

	if( !str || !*str )
		return 1;

	v = strtoll( str, NULL, 10 );

	// convert nsec to usec
	if( v > NSEC_THRESH )
		v /= 1000;

	// convert seconds to msec
	if( v < MSEC_THRESH )
		v *= 1000;

	// convert msec to usec
	if( v < USEC_THRESH)
		v *= 1000;

	if( usec )
		*usec = v;

	return 0;
}

#undef MSEC_THRESH
#undef USEC_THRESH
#undef NSEC_THRESH
