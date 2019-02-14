/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* log.c - handles log writing and log config                              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"

char *log_level_strings[LOG_LEVEL_MAX] =
{
	"FATAL",
	"ERROR",
	"WARN",
	"NOTICE",
	"INFO",
	"DEBUG"
};

int log_syslog_levels[LOG_LEVEL_MAX] =
{
	LOG_CRIT,
	LOG_ERR,
	LOG_WARNING,
	LOG_NOTICE,
	LOG_INFO,
	LOG_DEBUG
};

struct log_facility log_facilities[] =
{
	{ LOG_DAEMON, "daemon" },
	{ LOG_USER,   "user"   },
	{ LOG_LOCAL0, "local0" },
	{ LOG_LOCAL1, "local1" },
	{ LOG_LOCAL2, "local2" },
	{ LOG_LOCAL3, "local3" },
	{ LOG_LOCAL4, "local4" },
	{ LOG_LOCAL5, "local5" },
	{ LOG_LOCAL6, "local6" },
	{ LOG_LOCAL7, "local7" },
	{ 0,          NULL     }
};



int8_t log_get_level( char *str )
{
	int8_t i;

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


int log_get_facility( char *str )
{
	struct log_facility *lf;

	// step over the prefix
	if( !strncasecmp( str, "LOG_", 4 ) )
		str += 4;

	for( lf = log_facilities; lf->name; lf++ )
		if( !strcasecmp( str, lf->name ) )
			return lf->facility;

	return -1;
}



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


int log_line( int8_t level, const char *file, const int line, const char *fn, char *fmt, ... )
{
	char buf[LOG_LINE_MAX];
	int l = 0, d, tslen;
	va_list args;

	// check level
	if( level > _logger->level )
		return 0;

	d = ( level <= LOG_LEVEL_ERROR ) ? _logger->err_fd : _logger->ok_fd;

	// can we write?
	if( d < 0 )
		return -1;

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
	if( _logger->force_stdout && !_logger->use_std )
		printf( "%s", buf );

	// use syslog?
	if( _logger->use_syslog )
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

	return l;
}


int __log_open( void )
{
	if( ( _logger->ok_fd = open( _logger->filename, O_WRONLY|O_APPEND|O_CREAT, 0644 ) ) < 0 )
	{
		fprintf( stderr, "Unable to open log file '%s' -- %s\n",
			_logger->filename, Err );
		_logger->ok_fd = fileno( stdout );
		return -1;
	}
	// both go to file
	_logger->err_fd = _logger->ok_fd;

	return 0;
}


int __log_openlog( void )
{
	openlog( _logger->identifier, 0, _logger->facility );
	return 0;
}


int log_close( void )
{
	if( !_logger )
		return -1;

	if( _logger->use_std )
		return 0;

	if( _logger->use_syslog )
	{
		closelog( );
		return 0;
	}

	if( _logger->err_fd != _logger->ok_fd )
		close( _logger->err_fd );

	close( _logger->ok_fd );

	_logger->ok_fd  = fileno( stdout );
	_logger->err_fd = fileno( stderr );

	return 0;
}

// in response to a signal
void log_reopen( int sig )
{
	if( !_logger )
		return;

	if( _logger->use_std )
		return;

	log_close( );

	if( _logger->use_syslog )
		__log_openlog( );
	else
		__log_open( );

	if( _logger->notify_re )
		info( "Log file re-opened (version %s, uptime %.0f sec).", _proc->version, get_uptime( ) );
}



int log_start( void )
{
	if( !_logger )
		return -1;

	if( _logger->force_stdout )
		_logger->use_std = 1;

	if( _logger->use_std )
	{
		_logger->notify_re = 0;
		return 0;
	}

	if( _logger->use_syslog )
	{
		_logger->notify_re = 0;
		__log_openlog( );
		return 0;
	}

	return __log_open( );
}


// default log config
LOG_CTL *log_config_defaults( void )
{
	_logger = (LOG_CTL *) allocz( sizeof( LOG_CTL ) );

	// default to stdout
	_logger->filename       = strdup( "-" );
	_logger->level          = LOG_LEVEL_INFO;
	_logger->ok_fd          = fileno( stdout );
	_logger->err_fd         = fileno( stderr );
	_logger->use_std        = 1;
	_logger->write_level    = 1;
	_logger->force_stdout   = 0;
	_logger->notify_re      = 1;

	// syslog support
	_logger->use_syslog     = 0;
	_logger->identifier     = strdup( _proc->app_name );
	_logger->facility       = DEFAULT_LOG_FAC;

	return _logger;
}


int log_config_line( AVP *av )
{
	int lvl, fac;

	if( attIs( "filename" ) || attIs( "file" ) || attIs( "logfile" ) )
	{
	 	free( _logger->filename );
		_logger->filename = strdup( av->vptr );

		// are we still using stdout/stderr?
		_logger->use_std = ( strcmp( _logger->filename, "-" ) ) ? 0 : 1;

		// or how about syslog?
		_logger->use_syslog = ( strcasecmp( _logger->filename, "syslog" ) ) ? 0 : 1;
	}
	else if( attIs( "level" ) )
	{
		// get the log level
		lvl = log_get_level( av->vptr );
		if( _proc->run_flags & RUN_DEBUG )
			lvl = LOG_LEVEL_DEBUG;

		// and set it
		_logger->level = lvl;
	}
	else if( attIs( "writelevel" ) )
		_logger->write_level = config_bool( av );
	else if( attIs( "notify" ) )
		_logger->notify_re = config_bool( av );
	else if( attIs( "facility" ) )
	{
		if( ( fac = log_get_facility( av->vptr ) ) < 0 )
		{
			warn( "Unrecognised syslog facility '%s'", av->vptr );
			return -1;
		}
		_logger->facility = fac;
	}
	else if( attIs( "identifier" ) )
	{
		free( _logger->identifier );
		_logger->identifier = strdup( av->vptr );
	}
	else
		return -1;

	return 0;
}


