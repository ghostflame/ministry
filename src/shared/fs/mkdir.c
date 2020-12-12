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
* utils/mkdir.c - make directory function                                 *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"

int __fs_mkdir_recursive( const char *path, int len )
{
	struct stat sb;
	int en, ret;
	char *sl;

	errno = 0;

	// never mind dot
	if( len == 1 && *path == '.' )
		return 0;

	// slash at the end?  igore it
	if( *(path + len - 1) == '/' )
		--len;

	// don't create things inside /
	sl = memrchr( path, '/', len );
	if( sl == path )
	{
		debug( "Cowardly refusing to create '%s'.", path );
		return 0;
	}
	// but we do *need* some /'s
	else if( !sl )
	{
		errno = EINVAL;
		return errno;
	}

	// first question - does it exist?
	ret = stat( path, &sb );
	en  = errno;

	// did we succeed?
	if( ret == 0 && S_ISDIR( sb.st_mode ) )
		return 0;

	// scope block, avoid the big lump on the stack if we can
	{
		char dbuf[2048];
		int l, rv;

		// OK, try one level down
		l = sl - path;
		memcpy( dbuf, path, l );
		dbuf[l] = '\0';

		// prune trailing /
		while( l > 1 && dbuf[l-1] == '/' )
			dbuf[--l] = '\0';

		// recurse up towards /
		if( ( rv = __fs_mkdir_recursive( dbuf, l ) ) != 0 )
			return rv;
	}

	// ret 0 means the S_ISDIR test failed
	if( ret == 0 )
	{
		warn( "%s is not a directory.", path );
		errno = ENOTDIR;
		return errno;
	}

	// go back at look at the errno we got from stat
	if( en == ENOENT )
	{
		if( mkdir( path, 0755 ) )
		{
			ret = errno;

			// we get this when we add a slash on the end
			if( errno == EEXIST )
				return 0;

			warn( "Could not create dir %s -- %s",
				path, Err );
			return ret;
		}
		else
			debug( "Created dir %s", path );
	}
	else
	{
		ret = errno;
		warn( "Cannot stat dir %s -- %s", path, Err );
		return ret;
	}

	return 0;
}


int fs_mkdir_recursive( const char *path )
{
	int len;

	len = strlen( path );
	if( len > 2047 )
	{
		errno = ENAMETOOLONG;
		return errno;
	}

	return __fs_mkdir_recursive( path, len );
}


