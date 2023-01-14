/**************************************************************************
* Copyright 2015 John Denholm                                             *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
*                                                                         *
*                                                                         *
* utils/pidfile.c - utilities to handle pidfiles                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"


int fs_pidfile_mkdir( const char *filepath )
{
	char dbuf[2048], *s;
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


