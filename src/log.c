/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* log.c - handles log writing and log config                              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"

char *log_level_strings[LOG_LEVEL_MAX] =
{
	"FATAL",
	"ERR",
	"WARN",
	"NOTICE",
	"INFO",
	"DEBUG"
};

int log_get_level( char *str )
{
  	int i;

	if( !str || !*str )
	{
	  	warn( "Empty/null log level string." );
		// clearly, you need the help
		return LOG_LEVEL_DEBUG;
	}

	if( isdigit( *str ) )
	{
		i = atoi( str );
		if( i >= 0 && i < LOG_LEVEL_MAX )
		  	return i;

		warn( "Invalid log level string '%s'", str );
		return LOG_LEVEL_DEBUG;
	}

	for( i = LOG_LEVEL_FATAL; i < LOG_LEVEL_MAX; i++ )
		if( !strcasecmp( str, log_level_strings[i] ) )
		  	return i;


	warn( "Unrecognised log level string '%s'", str );
	return LOG_LEVEL_DEBUG;
}



int log_write_ts( char *to, int len )
{
	char tbuf[64];
	struct tm t;
	long usec;
	time_t sec;
#ifdef LOG_CURR_TIME

	sec  = ctl->curr_time.tv_sec;
	usec = ctl->curr_time.tv_nsec / 1000;
#else
	struct timeval tv;

	gettimeofday( &tv, NULL );
	sec  = tv.tv_sec;
	usec = (long) tv.tv_usec;
#endif

	localtime_r( &sec, &t );
	strftime( tbuf, 64, "%F %T", &t );

	return snprintf( to, len, "[%s.%06ld]", tbuf, usec );
}


int log_line( int level, const char *file, const int line, const char *fn, char *fmt, ... )
{
  	char buf[LOG_LINE_MAX];
	LOG_CTL *lc = ctl->log;
	va_list args;
	int l = 0;

	// check level
	if( level > lc->level )
		return 0;

	// can we write?
	if( lc->fd < 0 )
		return -1;

	// write the predictable parts
	l  = log_write_ts( buf, LOG_LINE_MAX );
	l += snprintf( buf + l, LOG_LINE_MAX - l, " [%s] ", log_level_strings[level] );


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
	if( lc->force_stdout )
		printf( "%s", buf );

	l = write( lc->fd, buf, l );

	// FATAL never returns
	if( level == LOG_LEVEL_FATAL )
	{
		// try to make sure we have that log line
		fsync( lc->fd );

		// and we're out
		shut_down( 1 );
	}

	return l;
}


int __log_open( void )
{
	// require a valid fd, and ! stderr
	if( ctl->log->fd >= 0
	 && ctl->log->fd != 2 )
		close( ctl->log->fd );

	if( ( ctl->log->fd = open( ctl->log->filename, O_WRONLY|O_APPEND|O_CREAT, 0644 ) ) < 0 )
	{
		fprintf( stderr, "Unable to open log file '%s' -- %s\n",
			ctl->log->filename, Err );
		return -1;
	}

	return 0;
}


int log_close( void )
{
	if( ctl
	 && ctl->log
	 && ctl->log->fd != 2 )
	{
		close( ctl->log->fd );
		ctl->log->fd = 2;
	}

	return 0;
}

// in response to a signal
void log_reopen( int sig )
{
	int nre = 0;

	if( ctl && ctl->log )
	{
		__log_open( );
		nre = ctl->log->notify_re;
	}

	if( nre )
		info( "Log file re-opened (version %s).", ctl->version );
}



int log_start( void )
{
	if( !ctl || !ctl->log )
		return -1;

	return __log_open( );
}


// default log config
LOG_CTL *log_config_defaults( void )
{
	LOG_CTL *l;

	l = (LOG_CTL *) allocz( sizeof( LOG_CTL ) );

	l->filename       = strdup( DEFAULT_LOG_FILE );
	l->level          = LOG_LEVEL_INFO;
	l->fd             = fileno( stderr );
	l->force_stdout   = 0;
	l->notify_re      = 1;

	return l;
}


int log_config_line( AVP *av )
{
	LOG_CTL *lc = ctl->log;
	int lvl;

	if( attIs( "filename" ) || attIs( "file" ) || attIs( "logfile" ) )
	{
	 	free( lc->filename );
		lc->filename = strdup( av->val );
	}
	else if( attIs( "level" ) )
	{
		// get the log level
		lvl = log_get_level( av->val );
		if( ctl->run_flags & RUN_DEBUG )
			lvl = LOG_LEVEL_DEBUG;

		// and set it
		lc->level = lvl;
	}
	else if( attIs( "notify" ) )
		lc->notify_re = config_bool( av );
	else
		return -1;

	return 0;
}


