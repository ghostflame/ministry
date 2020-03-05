/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* config/dir.c - handle include-dir directive                             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#include "local.h"


int config_handle_dir( char *path, WORDS *w )
{
	int ret = 0, count = 0, must = 1, nl;
	char fname[CONF_LINE_MAX];
	struct dirent *de;
	struct stat sb;
	char *p;
	DIR *d;

	if( !path || !*path )
	{
		warn( "Null directory handed by include-dir." );
		return 0;
	}

	if( *path == '?' )
	{
		++path;
		must = 0;
		debug( "Included directory '%s' is optional.", path );
	}

	if( stat( path, &sb ) )
	{
		if( errno == ENOENT )
		{
			if( must )
			{
				err( "Included directory '%s' does not exist.", path );
				return -1;
			}

			info( "Included (optional) directory '%s' does not exist, skipping.", path );
			return 0;
		}

		err( "Cannot stat include directory '%s' -- %s", path, Err );
		return -1;
	}

	if( !S_ISDIR( sb.st_mode ) )
	{
		if( must )
		{
			err( "Path '%s' is not a directory - cannot include-dir this path.", path );
			return -1;
		}

		info( "Path '%s' is not a directory - skipping include-dir.", path );
		return 0;
	}

	// add it to the watch tree
	fs_treemon_add( _proc->cfiles, path, 1 );

	debug( "Handling included directory '%s'", path );

	if( !( d = opendir( path ) ) )
	{
		err( "Could not read included directory '%s", path );
		return -1;
	}

	while( ( de = readdir( d ) ) != NULL )
	{
		if( snprintf( fname, CONF_LINE_MAX, "%s/%s", path, de->d_name ) == CONF_LINE_MAX )
		{
			warn( "Entry in include directory '%s' too long, skipping.", path );
			continue;
		}

		if( stat( fname, &sb ) )
		{
			warn( "Could not stat included file '%s, skipping.", fname );
			continue;
		}

		// ignore things that start with a dot - let's not have trouble
		if( *(de->d_name) == '.' )
		{
			debug( "Ignoring included entry '%s' (dot)", fname );
			continue;
		}

		// are we filtering this one out based on suffix check?
		if( chkcfFlag( SUFFIX ) && _proc->cfg_sffx_len )
		{
			nl = strlen( de->d_name );

			if( nl < ( _proc->cfg_sffx_len + 1 ) )
			{
				debug( "Ignoring included entry '%s' (non-matching, length)", fname );
				continue;
			}

			p = de->d_name + nl - _proc->cfg_sffx_len;

			if( memcmp( p, _proc->conf_sfx, _proc->cfg_sffx_len ) )
			{
				debug( "Ignoring included entry '%s', (non-matching, suffix)", fname );
				continue;
			}
		}

		// recurse?
		if( S_ISDIR( sb.st_mode ) )
		{
			// go deeper
			if( config_handle_dir( fname, w ) )
			{
				ret = -1;
				break;
			}

			continue;
		}

		// ok try what's left
		if( config_read( fname, w ) )
		{
			err( "Included (dir) config file '%s' is not valid.", fname );
			ret = -1;
			break;
		}

		++count;
	}

	closedir( d );

	debug( "Read %d files from included directory '%s'", count, path );
	return ret;
}


