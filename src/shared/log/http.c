/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* log/http.c - handles logging http interface                             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


int log_ctl_setdebug( HTREQ *req )
{
	int lvl = -1, both = 1;
	char *str = "dis";
	json_object *o;
	const char *s;
	LOGFL *lf;

	if( !( o = json_object_object_get( req->post->jo, "file" ) )
	 || !( s = json_object_get_string( o ) ) )
	{
		create_json_result( req->text, 0, "No valid file choice for %s", req->path->path );
		return 0;
	}

	if( !strcasecmp( s, "main" ) )
		lf = _logger->fps[0];
	else if( !strcasecmp( s, "http" ) )
		lf = _logger->fps[1];
	else
	{
		create_json_result( req->text, 0, "Unrecognised log file spec '%s'.", s );
		return 0;
	}

	if( !( o = json_object_object_get( req->post->jo, "debug" ) ) )
	{
		create_json_result( req->text, 0, "No debug on/off for set-debug." );
		return 0;
	}

	if( json_object_get_boolean( o ) )
	{
		lvl = LOG_LEVEL_DEBUG;
		both = 0;
		str = "en";
	}

	// set our new level
	if( log_file_set_level( lf, lvl, both ) )
	{
		notice( "Run-time %s debug logging %sabled.", lf->type, str );
		// and report success
		create_json_result( req->text, 1, "Run-time %s debug logging %sabled.", lf->type, str );
	}
	else
	{
		create_json_result( req->text, 1, "Log levels unaltered." );
	}
	return 0;
}


