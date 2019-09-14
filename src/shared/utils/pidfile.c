/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* utils/pidfile.c - utilities to handle pidfiles                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"


int recursive_mkdir( char *path )
{
	char dbuf[2048], *ls;
	struct stat sb;
	int l, ret;

	errno = 0;

	// we have a sensible looking path?
	if( !( ls = strrchr( path, '/' ) ) )
	{
		errno = EINVAL;
		return errno;
	}

	// are we being asked to create something within /
	// then just hand back 'ok' because we don't want
	// to do that
	if( ls == path )
	{
		debug( "Cowardly refusing to create '%s'.", path );
		return 0;
	}

	// path too long for us?
	if( ( l = ls - path ) > 2047 )
	{
		errno = ENAMETOOLONG;
		return errno;
	}

	memcpy( dbuf, path, l );
	dbuf[l] = '\0';

	// prune trailing /
	while( l > 1 && dbuf[l-1] == '/' )
		dbuf[--l] = '\0';

	// recurse up towards /
	if( ( ret = recursive_mkdir( dbuf ) ) != 0 )
		return ret;

	if( stat( path, &sb ) )
	{
		if( errno == ENOENT )
		{
			if( mkdir( path, 0755 ) )
			{
				ret = errno;
				warn( "Could not create dir %s -- %s",
					path, Err );
				return ret;
			}
			else
				info( "Created dir %s", path );
		}
		else
		{
			ret = errno;
			warn( "Cannot stat dir %s -- %s", path, Err );
			return ret;
		}
	}
	else if( !S_ISDIR( sb.st_mode ) )
	{
		warn( "%s is not a directory.", path );
		errno = ENOTDIR;
		return errno;
	}

	return 0;
}


int pidfile_mkdir( char *filepath )
{
	char dbuf[2028], *s;
	int l;

	if( !( s = strrchr( filepath, '/' ) ) )
	{
		warn( "Invalid pidfile spec: %s", filepath );
		return -1;
	}

	if( ( l = s - filepath ) > 2047 )
	{
		warn( "Pidfile path is too long, max length 2047." );
		return -2;
	}

	memcpy( dbuf, filepath, l );
	dbuf[l] = '\0';

	notice( "Trying to create dir '%s' for pidfile '%s'", dbuf, filepath );

	return recursive_mkdir( dbuf );
}



void pidfile_write( void )
{
	FILE *fh;

	// make our piddir
	if( recursive_mkdir( _proc->pidfile ) != 0 )
	{
		warn( "Unable to create pidfile directory." );
		return;
	}

	// and our file
	if( !( fh = fopen( _proc->pidfile, "w" ) ) )
	{
		warn( "Unable to write to pidfile %s -- %s",
			_proc->pidfile, Err );
		return;
	}
	fprintf( fh, "%d\n", getpid( ) );
	fclose( fh );
}

void pidfile_remove( void )
{
	if( unlink( _proc->pidfile ) && errno != ENOENT )
		warn( "Unable to remove pidfile %s -- %s",
			_proc->pidfile, Err );
}


