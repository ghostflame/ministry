/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* log/file.c - handles one logfile structure                              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



int log_file_open( LOGFL *lf )
{
	if( lf->use_std )
		return 0;

	if( lf->use_syslog )
		return log_open_syslog( );


	if( ( lf->ok_fd = open( lf->filename, O_WRONLY|O_APPEND|O_CREAT, 0644 ) ) < 0 )
	{
		fprintf( stderr, "Unable to open %s log file '%s' -- %s\n",
			lf->type, lf->filename, Err );
		lf->ok_fd = fileno( stdout );
		return -1;
	}

	// both go to file
	lf->err_fd = lf->ok_fd;

	return 0;
}



void log_file_close( LOGFL *lf )
{
	if( lf->use_std )
		return;

	if( lf->use_syslog )
	{
		log_close_syslog( );
		return;
	}

	if( lf->err_fd != lf->ok_fd )
		close( lf->err_fd );

	close( lf->ok_fd );

	lf->ok_fd  = fileno( stdout );
	lf->err_fd = fileno( stderr );
}



void log_file_reopen( LOGFL *lf )
{
	if( lf->use_std )
		return;

	log_file_close( lf );
	log_file_open( lf );
}



int log_file_start( LOGFL *lf )
{
	if( _logger->force_stdout )
		lf->use_std = 1;

	return log_file_open( lf );
}


int log_file_set_level( LOGFL *lf, int8_t level, int8_t both )
{
	int ret = 0;

	// never allowed
	if( level >= LOG_LEVEL_MAX )
		return -1;

	// used as a reset to orig
	if( level < 0 )
	{
		if( !both )
			return -1;

		if( lf->level != lf->orig_level )
		{
			lf->level = lf->orig_level;
			ret = 1;
		}
	}
	else
	{
		if( lf->level != level )
		{
			lf->level = level;
			ret = 1;
		}

		if( both && lf->orig_level != level )
		{
			lf->orig_level = level;
			ret = 1;
		}
	}

	return ret;
}


