/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* log/log.c - handles log writing and log config                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


int log_write_ts( char *to, int len )
{
	char tbuf[64];
	struct tm t;
	long usec;
	time_t sec;
#ifdef LOG_CURR_TIME

	sec  = _proc->curr_time.tv_sec;
	usec = _proc->curr_time.tv_nsec / 1000;
#else
	struct timeval tv;

	gettimeofday( &tv, NULL );
	sec  = tv.tv_sec;
	usec = (long) tv.tv_usec;
#endif

	localtime_r( &sec, &t );
	strftime( tbuf, 64, "%F %T", &t );

	return snprintf( to, len, "[%s.%06ld] ", tbuf, usec );
}


int log_line( int fi, int8_t level, const char *file, const int line, const char *fn, char *fmt, ... )
{
	char buf[LOG_LINE_MAX];
	int l = 0, d, tslen;
	va_list args;
	LOGFL *lf;

	lf = _logger->fps[fi];

	// check level
	if( level > lf->level )
		return 0;

	d = ( level <= LOG_LEVEL_ERROR ) ? lf->err_fd : lf->ok_fd;

	// can we write?
	if( d < 0 )
		return -1;

	// keep score
	++(_logger->counts[level]);

	// capture the timestamp length, we won't write it to syslog
	l += log_write_ts( buf, LOG_LINE_MAX );
	tslen = l;

	// and if we don't want the log level either, step over that
	l += snprintf( buf + l, LOG_LINE_MAX - l, "[%s] ", log_level_strings[level] );
	if( !_logger->write_level )
		tslen = l;

	// fatals, errs and debugs get details
	switch( level )
	{
		case LOG_LEVEL_FATAL:
		case LOG_LEVEL_DEBUG:
			l += snprintf( buf + l, LOG_LINE_MAX - l,
					"[%s:%d:%s] ", file, line, fn );
			break;
	}

	// and we're off
	va_start( args, fmt );
	l += vsnprintf( buf + l, LOG_LINE_MAX - l, fmt, args );
	va_end( args );

	// add a newline
	if( buf[l - 1] != '\n' )
	{
		buf[l++] = '\n';
		buf[l]   = '\0';
	}

	// maybe always log to stdout?
	if( _logger->force_stdout && !lf->use_std )
		printf( "%s", buf );

	// use syslog?
	if( lf->use_syslog )
	{
		syslog( _logger->facility|log_syslog_levels[level], "%s", buf + tslen );
		return l;
	}

	// OK, on to the file then
	l = write( d, buf, l );

	// FATAL never returns
	if( level == LOG_LEVEL_FATAL )
	{
		// try to make sure we have that log line
		fsync( d );

		// and we're out
		app_finish( 1 );
	}

	lf->size += l;

	return l;
}



