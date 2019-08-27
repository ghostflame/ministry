/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* utils/pidfile.c - utilities to handle pidfiles                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"


void pidfile_mkdir( char *path )
{
	char dbuf[2048], *ls;
	struct stat sb;
	int l;

	// we have a sensible looking path?
	if( !( ls = strrchr( path, '/' ) )
	 || ls == path )
		return;

	// path too long for us?
	if( ( l = ls - path ) > 2047 )
		return;

	memcpy( dbuf, path, l );
	dbuf[l] = '\0';

	// prune trailing /
	while( l > 1 && dbuf[l-1] == '/' )
		dbuf[--l] = '\0';

	// recurse up towards /
	pidfile_mkdir( dbuf );

	if( stat( dbuf, &sb ) )
	{
		if( errno == ENOENT )
		{
			if( mkdir( dbuf, 0755 ) )
				warn( "Could not create pidfile path parent dir %s -- %s",
					dbuf, Err );
			else
				info( "Created pidfile path parent dir %s", dbuf );
		}
		else
			warn( "Cannot stat pidfile parent dir %s -- %s", dbuf, Err );
	}
	else if( !S_ISDIR( sb.st_mode ) )
	{
		warn( "Pidfile parent %s is not a directory.", dbuf );
	}
}



void pidfile_write( void )
{
	FILE *fh;

	// make our piddir
	pidfile_mkdir( _proc->pidfile );

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


