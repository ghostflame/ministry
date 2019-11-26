/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* log/conf.c - handles log config                                         *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

LOG_CTL *_logger = NULL;

const char *log_level_strings[LOG_LEVEL_MAX] =
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
		i = (int8_t) strtol( str, NULL, 10 );
		if( i >= 0 && i < LOG_LEVEL_MAX )
		  	return i;

		warn( "Invalid log level string '%s'", str );
		return LOG_LEVEL_DEBUG;
	}

	for( i = LOG_LEVEL_MIN; i < LOG_LEVEL_MAX; ++i )
		if( !strcasecmp( str, log_level_strings[i] ) )
		  	return i;


	warn( "Unrecognised log level string '%s'", str );
	return LOG_LEVEL_DEBUG;
}

const char *log_get_level_name( int8_t level )
{
	if( level >= LOG_LEVEL_MIN && level < LOG_LEVEL_MAX )
		return log_level_strings[level];

	return "unknown";
}


void log_set_force_stdout( int set )
{
	_logger->force_stdout = ( set ) ? 1 : 0;
}


int log_get_facility( char *str )
{
	struct log_facility *lf;

	// step over the prefix
	if( !strncasecmp( str, "LOG_", 4 ) )
		str += 4;

	for( lf = log_facilities; lf->name; ++lf )
		if( !strcasecmp( str, lf->name ) )
			return lf->facility;

	return -1;
}





// default log config
LOG_CTL *log_config_defaults( void )
{
	LOGFL *m, *h;

	_logger = (LOG_CTL *) allocz( sizeof( LOG_CTL ) );

	// main file - stdout by default
	m               = (LOGFL *) allocz( sizeof( LOGFL ) );
	m->filename     = strdup( "-" );
	m->type         = "main";
	m->level        = LOG_LEVEL_INFO;
	m->orig_level   = m->level;
	m->use_std      = 1;
	m->ok_fd        = fileno( stdout );
	m->err_fd       = fileno( stderr );

	// http file - stdout by default
	h               = (LOGFL *) allocz( sizeof( LOGFL ) );
	h->filename     = strdup( "-" );
	h->type         = "http";
	h->level        = LOG_LEVEL_INFO;
	h->orig_level   = h->level;
	h->use_std      = 1;
	h->ok_fd        = fileno( stdout );
	h->err_fd       = fileno( stderr );

	_logger->main   = m;
	_logger->http   = h;
	_logger->fps    = (LOGFL **) allocz( 2 * sizeof( LOGFL * ) );

	_logger->fps[0] = _logger->main;
	_logger->fps[1] = _logger->http;

	// default to stdout
	_logger->write_level    = 1;
	_logger->force_stdout   = 0;
	_logger->notify_re      = 1;

	// syslog support
	_logger->identifier     = strdup( _proc->app_name );
	_logger->facility       = DEFAULT_LOG_FAC;

	// unification - unified by default
	_logger->unified        = 1;


	return _logger;
}


int log_config_line( AVP *av )
{
	LOGFL *l = _logger->main;
	int lvl, fac;

	if( !strncasecmp( av->aptr, "http.", 5 ) )
	{
		av->alen -= 5;
		av->aptr += 5;

		l = _logger->http;
	}
	else if( !strncasecmp( av->aptr, "main.", 5 ) )
	{
		av->alen -= 5;
		av->aptr += 5;

		l = _logger->main;
	}

	if( attIs( "filename" ) || attIs( "file" ) || attIs( "logfile" ) )
	{
	 	free( l->filename );
		l->filename = strdup( av->vptr );

		// are we still using stdout/stderr?
		l->use_std = ( strcmp( l->filename, "-" ) ) ? 0 : 1;

		// or how about syslog?
		l->use_syslog = ( strcasecmp( l->filename, "syslog" ) ) ? 0 : 1;
	}
	else if( attIs( "level" ) )
	{
		// get the log level
		lvl = log_get_level( av->vptr );
		if( _proc->run_flags & RUN_DEBUG )
			lvl = LOG_LEVEL_DEBUG;

		// and set it
		l->level = lvl;
	}
	else if( attIs( "unified" ) )
	{
		_logger->unified = config_bool( av );
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


