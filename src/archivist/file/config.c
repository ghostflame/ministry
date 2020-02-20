/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* file/config.c - file configuration functions                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

FILE_CTL *_file = NULL;

RKBKT file_base_default_buckets[3] =
{
	{
		.period = 10000000,	// 10s
		.count  = 25920		// 3d
	},
	{
		.period = 60000000, // 60s
		.count  = 43200		// 30d
	},
	{	// we need this to end the list
		.period = 0,
		.count  = 0
	}
};



int file_set_bucket( int64_t period, int64_t count, int which )
{
	int i;

	if( which < 0 )
	{
		for( i = 0; i < RKV_FL_MAX_BUCKETS; ++i )
			if( _file->default_buckets[i].period == 0 )
			{
				which = i;
				break;
			}

		if( i == RKV_FL_MAX_BUCKETS )
		{
			err( "Cannot add another bucket - max of %d reached.",
				RKV_FL_MAX_BUCKETS );
			return -1;
		}
	}

	_file->default_buckets[which].period = period;
	_file->default_buckets[which].count  = count;

	// count them
	for( i = 0; i < RKV_FL_MAX_BUCKETS; ++i )
		if( _file->default_buckets[i].count == 0 )
			break;

	_file->bktct = i;

	return 0;
}


void file_set_base_path( char *path, int len )
{
	if( !len )
		len = strlen( path );

	if( _file->base_path )
		free( _file->base_path );

	_file->base_path = (char *) allocz( len + 2 );
	memcpy( _file->base_path, path, len );

	if( path[len-1] != '/' )
		_file->base_path[len++] = '/';

	_file->bplen = len;
}


int file_init( void )
{
	BUF *fbuf, *pbuf;
	int64_t tsa, fc;
	double diff;
	RKBKT *b;
	int8_t i;

	if( _file->bktct == 0 )
	{
		for( i = 0; i < RKV_FL_MAX_BUCKETS; ++i )
		{
			b = file_base_default_buckets + i;
			if( !b->count )
				break;

			file_set_bucket( b->period, b->count, -1 );
		}
	}

	if( fs_mkdir_recursive( _file->base_path ) != 0 )
	{
		err( "Could not find file base path %s -- %s",
			_file->base_path, Err );
		return -1;
	}

	// make sure we don't make crazy paths allowed
	_file->maxpath = 2047 - FILE_EXTENSION_LEN - _file->bplen;

	// scan existing files
	fbuf = strbuf( _file->maxpath );
	pbuf = strbuf( _file->maxpath );
	strbuf_copy( fbuf, _file->base_path, _file->bplen );

	// scan our files, work out how long it took
	tsa = get_time64( );
	fc = file_scan_dir( fbuf, pbuf, ctl->tree->root, 0 );
	diff = (double) ( get_time64( ) - tsa );
	diff /= MILLIONF;

	info( "Scanned %ld files in %.6f sec.", fc, diff );

	// and start our writers
	for( i = 0; i < _file->wr_threads; ++i )
		thread_throw_named_f( &file_writer, NULL, i, "file_writer_%d", i );

	return 0;
}



FILE_CTL *file_config_defaults( void )
{
	_file = (FILE_CTL *) allocz( sizeof( FILE_CTL ) );

	file_set_base_path( DEFAULT_BASE_PATH, 0 );

	_file->wr_threads   = FILE_DEFAULT_THREADS;
	_file->ms_sync      = MS_ASYNC;	// asynchronous msync on close
	_file->max_open_sec = FILE_MAX_OPEN_SEC;

	return _file;
}


int file_config_line( AVP *av )
{
	int64_t v, w;
	char *sp;

	if( attIs( "basePath" ) || attIs( "filePath" ) )
	{
		file_set_base_path( av->vptr, av->vlen );
	}
	else if( attIs( "bucket" ) )
	{
		if( !( sp = memchr( av->vptr, ' ', av->vlen ) ) )
		{
			warn( "Invalid bucket spec: <period (ms)> <count>" );
			return -1;
		}

		*sp++ = '\0';

		if( parse_number( av->vptr, &v, NULL ) == NUM_INVALID )
		{
			warn( "Invalid bucket spec: <period (ms)> <count>" );
			return -1;
		}
		if( parse_number( sp, &w, NULL ) == NUM_INVALID )
		{
			warn( "Invalid bucket spec: <period (ms)> <count>" );
			return -1;
		}

		if( !v || !w )
		{
			warn( "Invalid bucket spec: <period (ms)> <count>" );
			return -1;
		}

		file_set_bucket( v, w, -1 );
	}
	else if( attIs( "ioThreads" ) || attIs( "threads" ) )
	{
		if( parse_number( av->vptr, &v, NULL ) == NUM_INVALID )
		{
			warn( "Invalid writer threads count." );
			return -1;
		}

		if( v < 1 || v > 32 )
		{
			warn( "Writer threads must be 1 < x 32" );
			return -1;
		}

		_file->wr_threads = (int8_t) v;
	}
	else if( attIs( "sync" ) )
	{
		if( config_bool( av ) )
		{
			_file->ms_sync = MS_SYNC;
			notice( "Synchronous write on munmap enabled." );
		}
		else
			_file->ms_sync = MS_ASYNC;
	}
	else if( attIs( "maxFileOpenSec" ) )
	{
		if( parse_number( av->vptr, &v, NULL ) == NUM_INVALID )
		{
			err( "Invalid max seconds to hold open inactive files: %s", av->vptr );
			return -1;
		}

		_file->max_open_sec = v;
	}
	else
		return -1;

	return 0;
}
