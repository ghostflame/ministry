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
* utils/utils.c - utility routines                                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"



// random number below n
int64_t get_rand( int64_t n )
{
	double d;

	d  = (double) random( ) / ( 1.0 * RAND_MAX );
	d *= (double) n;

	return (int64_t) d;
}


int8_t percent( void )
{
	return 1 + (int8_t) get_rand( 100 );
}





int setlimit( int res, int64_t val )
{
	char *which = "unknown";
	struct rlimit rl;

	switch( res )
	{
		case RLIMIT_NOFILE:
			which = "files";
			break;
		case RLIMIT_NPROC:
			which = "procs";
			break;
	}

	getrlimit( res, &rl );

	// -1 means GIVE ME EVERYTHING!
	if( val == -1 )
		rl.rlim_cur = rl.rlim_max;
	else
	{
		if( (int64_t) rl.rlim_max < val )
		{
			err( "Limit %s max is %ld.", which, rl.rlim_max );
			return -1;
		}
		rl.rlim_cur = val;
	}

	if( setrlimit( res, &rl ) )
	{
		err( "Cannot set %s limit to %ld: %s",
			which, val, Err );
		return -1;
	}

	debug( "Set %s limit to %ld.", which, rl.rlim_cur );
	return 0;
}


uint64_t lockless_fetch( LLCT *l )
{
	uint64_t diff, curr;

	// we can only read from it *once*
	// then we do maths
	curr = l->count;
	diff = curr - l->prev;

	// and set the previous
	l->prev = curr;

	return diff;
}


int read_file( const char *path, char **buf, int *len, int perm, const char *desc )
{
	int l, max, r, fd;
	struct stat sb;

	if( !buf || !len )
	{
		err( "Need a buffer and length parameter in read_file." );
		return 1;
	}

	max = *len;

	if( !desc )
		desc = "file";

	if( !path )
	{
		err( "No %s path.", desc );
		return -1;
	}

	if( stat( path, &sb ) )
	{
		err( "Cannot stat %s %s: %s", desc, path, Err );
		return -2;
	}

	if( !S_ISREG( sb.st_mode ) )
	{
		err( "Mode of %s %s is not a regular file.", desc, path );
		return -3;
	}

	l = (int) sb.st_size;

	// files in /proc stat as 0 bytes, helpfully
	if( l > 0 )
		*len = l;

	if( l > max )
	{
		err( "Size of %s %s is too big at %d bytes.", desc, path, l );
		return -4;
	}

	if( ( fd = open( path, O_RDONLY|O_CLOEXEC ) ) < 0 )
	{
		err( "Cannot open %s %s: %s", desc, path, Err );
		return -5;
	}

	if( !*buf )
	{
		// we need a buffer if you want to read from /proc
		*buf = ( perm ) ? mem_perm( l + 1 ) : allocz( l + 1 );
		debug( "Creating buffer of %d bytes for %s.", 1 + *len, desc );
	}
	else
		memset( *buf, 0, max );

	// with /proc we just read what we can get
	if( l == 0 && max > 0 )
		r = max;
	else
		r = l;

	if( ( *len = read( fd, *buf, r ) ) < 0 )
	{
		err( "Could not read %s %s: %s", desc, path, Err );
		close( fd );
		*len = 0;
		return -6;
	}

	close( fd );
	return 0;
}


int parse_number( const char *str, int64_t *iv, double *dv )
{
	if( iv )
		*iv = 0;
	if( dv )
		*dv = 0;

	if( !strncmp( str, "0x", 2 ) )
	{
		if( iv )
			*iv = strtoll( str, NULL, 16 );
		return NUM_HEX;
	}

	while( *str == '+' )
		++str;

	if( *str != '-' && !isdigit( *str ) )
		return NUM_INVALID;

	if( strchr( str, '.' ) )
	{
		if( dv )
			*dv = strtod( str, NULL );
		return NUM_FLOAT;
	}

	if( strlen( str ) > 1 && *str == '0' )
	{
		if( iv )
			*iv = strtoll( str, NULL, 8 );
		return NUM_OCTAL;
	}

	if( iv )
		*iv = strtoll( str, NULL, 10 );
	if( dv )
		*dv = strtod( str, NULL );

	return NUM_NORMAL;
}


const char *hash_size_names[8] =
{
	"nano",
	"micro",
	"tiny",
	"small",
	"medium",
	"large",
	"xlarge",
	"x2large",
};

uint64_t hash_size_vals[8] =
{
	MEM_HSZ_NANO,
	MEM_HSZ_MICRO,
	MEM_HSZ_TINY,
	MEM_HSZ_SMALL,
	MEM_HSZ_MEDIUM,
	MEM_HSZ_LARGE,
	MEM_HSZ_XLARGE,
	MEM_HSZ_X2LARGE
};


uint64_t hash_size( const char *str )
{
	int64_t v = 0;
	int i;

	if( !str || !strlen( str ) )
	{
		warn( "Invalid hashtable size string." );
		return 0;
	}

	if( ( i = str_search( str, hash_size_names, 8 ) ) >= 0 )
		return hash_size_vals[i];

	if( parse_number( str, &v, NULL ) == NUM_INVALID )
	{
		warn( "Unrecognised hash table size '%s'", str );
		return 0;
	}

	return (uint64_t) v;
}




int is_url( const char *str )
{
	int l = strlen( str );

	if( l < 8 )
		return STR_URL_NO;

	if( !strncasecmp( str, "http://", 7 ) )
		return STR_URL_YES;

	if( !strncasecmp( str, "https://", 8 ) )
		return STR_URL_SSL;

	return STR_URL_NO;
}



