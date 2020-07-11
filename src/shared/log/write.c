/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* log/log.c - handles log writing and log config                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


void log_write_ts( BUF *b )
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

	strbuf_printf( b, "[%s.%06ld] ", tbuf, usec );
}


int log_line( int fi, int8_t level, const char *file, const int line, const char *fn, char *fmt, ... )
{
	char buf[LOG_LINE_MAX];
	va_list args;
	BUF bf, *b;
	LOGFL *lf;
	int l, d;

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

	bf.len = 0;
	bf.sz  = LOG_LINE_MAX;
	bf.buf = buf;
	b      = &bf;

	if( !lf->use_syslog )
	{
		log_write_ts( b );

		if( _logger->write_level )
			strbuf_aprintf( b, "[%s] ", log_level_strings[level] );
	}

	// fatals, errs and debugs get details
	switch( level )
	{
		case LOG_LEVEL_FATAL:
		case LOG_LEVEL_DEBUG:
			strbuf_aprintf( b, "[%s:%d:%s] ", file, line, fn );
			break;
	}

	// and we're off
	va_start( args, fmt );
	strbuf_avprintf( b, fmt, args );
	va_end( args );

	// add a newline
	if( strbuf_lastchar( b ) != '\n' )
	{
		buf_addchar( b, '\n' );
		buf_terminate( b );
	}

	// maybe always log to stdout?
	if( _logger->force_stdout && !lf->use_std )
		printf( "%s", b->buf );

	// use syslog?
	if( lf->use_syslog )
	{
		syslog( _logger->facility|log_syslog_levels[level], "%s", b->buf );
		return b->len;
	}

	// OK, on to the file then
	l = write( d, b->buf, b->len );

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



