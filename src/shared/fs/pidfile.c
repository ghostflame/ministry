/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* utils/pidfile.c - utilities to handle pidfiles                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"


int fs_pidfile_mkdir( const char *filepath )
{
	char dbuf[2028], *s;
	int l;

	// are we right there?
	if( !strcmp( filepath, "." ) )
		return 0;

	if( !( s = strrchr( filepath, '/' ) ) )
	{
		warn( "Invalid directory spec: %s", filepath );
		return -1;
	}

	if( ( l = ( s - filepath ) ) > 2047 )
	{
		warn( "Pidfile path is too long, max length 2047." );
		return -2;
	}

	memcpy( dbuf, filepath, l );
	dbuf[l] = '\0';

	notice( "Trying to create dir '%s' for pidfile '%s'", dbuf, filepath );

	return fs_mkdir_recursive( dbuf );
}



void fs_pidfile_write( void )
{
	FILE *fh;

	// make our piddir
	if( fs_pidfile_mkdir( _proc->pidfile ) != 0 )
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


void fs_pidfile_remove( void )
{
	if( unlink( _proc->pidfile ) && errno != ENOENT )
		warn( "Unable to remove pidfile %s -- %s",
			_proc->pidfile, Err );
}


