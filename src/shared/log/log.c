/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* log/log.c - handles log writing and log config                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"




int log_open_syslog( void )
{
	if( !_logger->sl_open )
	{
		openlog( _logger->identifier, 0, _logger->facility );
		_logger->sl_open = 1;
	}

	return 0;
}

int log_close_syslog( void )
{
	if( _logger->sl_open )
	{
		closelog( );
		_logger->sl_open = 0;
	}

	return 0;
}



int log_close( void )
{
	if( !_logger )
		return -1;

	if( !_logger->unified )
		log_file_close( _logger->http );

	log_file_close( _logger->main );

	return 0;
}



// in response to a signal
void log_reopen( int sig )
{
	double d;

	if( !_logger )
		return;

	if( !_logger->unified )
		log_file_reopen( _logger->http );

	log_file_reopen( _logger->main );

	// are we writing a first line?
	if( _logger->notify_re )
	{
		d = get_uptime( );

		if( !_logger->main->use_std )
			info( "Main log file re-opened (version %s, uptime %.0f sec).", _proc->version, d );

		if( !_logger->unified && _logger->http->use_std )
			hinfo( "HTTP log file re-opened (version %s, uptime %.0f sec).", _proc->version, d );
	}
}



int log_start( void )
{
	int ret = 0;

	// add a callback to toggle debugging
	http_add_control( "set-debug", "Set/unset debug logging", NULL, log_ctl_setdebug, NULL, 0 );

	if( !_logger )
		return -1;

	if( !_logger->main->use_std )
		debug( "Starting logging - no more main logs to stdout." );

	_logger->fps[1] = ( _logger->unified ) ? _logger->main : _logger->http;

	ret += log_file_start( _logger->main );

	if( !_logger->unified )
		ret += log_file_start( _logger->http );

	return ret;
}

// 1 - did something
// 0 - did nothing
int log_set_level( int8_t level, int8_t both )
{
	if( log_file_set_level( _logger->main, level, both ) > 0
	 && log_file_set_level( _logger->http, level, both ) > 0 )
		return 1;

	return 0;
}

