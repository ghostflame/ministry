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




void file_set_base_path( const char *path, int len )
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




int file_parse_buckets( RKBMT *m, const char *str, int len )
{
	int64_t period, count;
	WORDS w, v;
	char *copy;
	int i, ret;

	copy = str_copy( str, len );
	ret  = -1;

	strwords( &w, copy, len, ';' );

	if( w.wc >= RKV_FL_MAX_BUCKETS )
	{
		err( "Too many buckets specified (%d), max is %d.", w.wc, RKV_FL_MAX_BUCKETS );
		goto PARSE_DONE;
	}

	for( i = 0; i < w.wc; ++i )
	{
		if( strwords( &v, w.wd[i], w.len[i], ':' ) != 2 )
		{
			err( "Invalid retention bucket spec (set %d): %s", i, str );
			goto PARSE_DONE;
		}

		if( time_span_usec( v.wd[0], &period ) != 0 )
		{
			err( "Invalid retention bucket spec item: %s", v.wd[0] );
			goto PARSE_DONE;
		}

		if( time_span_usec( v.wd[1], &count ) != 0 )
		{
			err( "Invalid retention bucket spec item: %s", v.wd[1] );
			goto PARSE_DONE;
		}

		// count is divided by period to get slots
		count /= period;

		m->buckets[m->bct].period = period;
		m->buckets[m->bct].count  = count;

		debug( "Found bucket %d * %d", period, count );
		++(m->bct);
	}

	ret = 0;

PARSE_DONE:
	free( copy );
	return ret;
}




int file_init( void )
{
	BUF *fbuf, *pbuf;
	int64_t tsa, fc;
	double diff;
	RKBMT *m;
	int8_t i;

	// add a default retention
	if( _file->rblocks == 0 )
	{
		warn( "No retention config: adding default of %s", RKV_DEFAULT_RET );

		m = (RKBMT *) allocz( sizeof( RKBMT ) );
		m->is_default = 1;
		m->name = str_perm( "Default bucket block", 0 );
		file_parse_buckets( m, RKV_DEFAULT_RET, strlen( RKV_DEFAULT_RET ) );

		_file->ret_default = m;
		_file->retentions  = m;
		_file->rblocks     = 1;
	}
	else
	{
		// fix the order
		_file->retentions = (RKBMT *) mem_reverse_list( _file->retentions );

		// check we do have a default
		if( !_file->ret_default )
		{
			// choose the last one
			for( m = _file->retentions; m->next; m = m->next );

			_file->ret_default = m;
			notice( "Chose retention block %s as default.", m->name );
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
	diff /= BILLIONF;

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


static RKBMT __file_config_bkt_match;
static int __file_config_bkt_match_state = 0;

int file_config_line( AVP *av )
{
	RKBMT *n, *m = &__file_config_bkt_match;
	int64_t v;
	char *d;

	if( __file_config_bkt_match_state == 0 )
	{
		memset( m, 0, sizeof( RKBMT ) );
	}


	if( !( d = memchr( av->aptr, '.', av->alen ) ) )
	{
		if( attIs( "basePath" ) || attIs( "filePath" ) )
		{
			file_set_base_path( av->vptr, av->vlen );
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

			_file->wr_threads = v;
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

	*d++ = '\0';

	if( attIs( "retention" ) )
	{
		av->alen -= d - av->aptr;
		av->aptr += d - av->aptr;

		if( attIs( "buckets" ) )
		{
			__file_config_bkt_match_state = 1;
			return file_parse_buckets( m, av->vptr, av->vlen );
		}
		else if( attIs( "name" ) )
		{
			__file_config_bkt_match_state = 1;
			if( m->name )
			{
				err( "Retention block %s already has a name.", m->name );
				return -1;
			}

			m->name = str_perm( av->vptr, av->vlen );
		}
		else if( attIs( "match" ) )
		{
			__file_config_bkt_match_state = 1;

			if( !m->rgx )
				m->rgx = regex_list_create( 1 );

			return regex_list_add( av->vptr, 0, m->rgx );
		}
		else if( attIs( "fallbackMatch" ) )
		{
			__file_config_bkt_match_state = 1;

			if( !m->rgx )
				m->rgx = regex_list_create( 1 );

			return regex_list_add( av->vptr, 1, m->rgx );
		}
		else if( attIs( "default" ) )
		{
			__file_config_bkt_match_state = 1;
			m->is_default = config_bool( av );

			if( m->is_default && _file->ret_default )
			{
				err( "Cannot have two default retention blocks - %s is already set as deault.", _file->ret_default->name );
				return -1;
			}
		}
		else if( attIs( "done" ) )
		{
			if( !m->name )
			{
				m->name = mem_perm( 24 );
				snprintf( m->name, 24, "retention block %d", _file->rblocks );
			}

			if( !m->is_default && !m->rgx )
			{
				err( "No match patterns assigned to retention block %s", m->name );
				return -1;
			}

			if( !m->bct )
			{
				err( "No retention buckets assigned to retention block %s", m->name );
				return -1;
			}

			n = (RKBMT *) allocz( sizeof( RKBMT ) );
			memcpy( n, m, sizeof( RKBMT ) );
			memset( m, 0, sizeof( RKBMT ) );

			// put it into the list
			n->next = _file->retentions;
			_file->retentions = n;
			++(_file->rblocks);

			if( n->is_default )
				_file->ret_default = n;

			__file_config_bkt_match_state = 0;
		}
		else
			return -1;
	}



	return 0;
}
