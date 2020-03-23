/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* rkv/config.c - file configuration functions                             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

RKV_CTL *_rkv = NULL;




void rkv_set_base_path( const char *path, int len )
{
	if( !len )
		len = strlen( path );

	if( _rkv->base_path )
		free( _rkv->base_path );

	_rkv->base_path = (char *) allocz( len + 2 );
	memcpy( _rkv->base_path, path, len );

	if( path[len-1] != '/' )
		_rkv->base_path[len++] = '/';

	_rkv->bplen = len;
}



int rkv_parse_buckets( RKBMT *m, const char *str, int len )
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




int rkv_init( void )
{
	BUF *fbuf, *pbuf;
	int64_t tsa, fc;
	double diff;
	RKBMT *m;
	//int8_t i;

	// add a default retention
	if( _rkv->rblocks == 0 )
	{
		warn( "No retention config: adding default of %s", RKV_DEFAULT_RET );

		m = (RKBMT *) allocz( sizeof( RKBMT ) );
		m->is_default = 1;
		m->name = str_perm( "Default bucket block", 0 );
		rkv_parse_buckets( m, RKV_DEFAULT_RET, strlen( RKV_DEFAULT_RET ) );

		_rkv->ret_default = m;
		_rkv->retentions  = m;
		_rkv->rblocks     = 1;
	}
	else
	{
		// fix the order
		_rkv->retentions = (RKBMT *) mem_reverse_list( _rkv->retentions );

		// check we do have a default
		if( !_rkv->ret_default )
		{
			// choose the last one
			for( m = _rkv->retentions; m->next; m = m->next );

			_rkv->ret_default = m;
			notice( "Chose retention block %s as default.", m->name );
		}
	}

	if( fs_mkdir_recursive( _rkv->base_path ) != 0 )
	{
		err( "Could not find file base path %s -- %s",
			_rkv->base_path, Err );
		return -1;
	}

	// make sure we don't make crazy paths allowed
	_rkv->maxpath = 2047 - RKV_EXTENSION_LEN - _rkv->bplen;

	// scan existing files
	fbuf = strbuf( _rkv->maxpath );
	pbuf = strbuf( _rkv->maxpath );
	strbuf_copy( fbuf, _rkv->base_path, _rkv->bplen );

	// scan our files, work out how long it took
	tsa = get_time64( );
	// needs moving back into archivist
	//fc = rkv_scan_dir( fbuf, pbuf, 0, ctl->tree->root, &tree_insert_leaf, &tree_insert_node );
	fc = 0;
	diff = (double) ( get_time64( ) - tsa );
	diff /= BILLIONF;

	info( "Scanned %ld files in %.6f sec.", fc, diff );

	// and start our writers
	// needs moving back into archivist, for the tree hash
	//for( i = 0; i < _rkv->wr_threads; ++i )
	//	thread_throw_named_f( &rkv_writer, ctl->tree->hash, i, "rkv_writer_%d", i );

	return 0;
}



RKV_CTL *rkv_config_defaults( void )
{
	_rkv = (RKV_CTL *) allocz( sizeof( RKV_CTL ) );

	rkv_set_base_path( DEFAULT_BASE_PATH, 0 );

	_rkv->wr_threads   = RKV_DEFAULT_THREADS;
	_rkv->ms_sync      = MS_ASYNC;	// asynchronous msync on close
	_rkv->max_open_sec = RKV_MAX_OPEN_SEC;

	return _rkv;
}


static RKBMT __rkv_config_bkt_match;
static int __rkv_config_bkt_match_state = 0;

int rkv_config_line( AVP *av )
{
	RKBMT *n, *m = &__rkv_config_bkt_match;
	int64_t v;
	char *d;

	if( __rkv_config_bkt_match_state == 0 )
	{
		memset( m, 0, sizeof( RKBMT ) );
	}


	if( !( d = memchr( av->aptr, '.', av->alen ) ) )
	{
		if( attIs( "basePath" ) || attIs( "filePath" ) )
		{
			rkv_set_base_path( av->vptr, av->vlen );
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

			_rkv->wr_threads = v;
		}
		else if( attIs( "sync" ) )
		{
			if( config_bool( av ) )
			{
				_rkv->ms_sync = MS_SYNC;
				notice( "Synchronous write on munmap enabled." );
			}
			else
				_rkv->ms_sync = MS_ASYNC;
		}
		else if( attIs( "maxFileOpenSec" ) )
		{
			if( parse_number( av->vptr, &v, NULL ) == NUM_INVALID )
			{
				err( "Invalid max seconds to hold open inactive files: %s", av->vptr );
				return -1;
			}

			_rkv->max_open_sec = v;
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
			__rkv_config_bkt_match_state = 1;
			return rkv_parse_buckets( m, av->vptr, av->vlen );
		}
		else if( attIs( "name" ) )
		{
			__rkv_config_bkt_match_state = 1;
			if( m->name )
			{
				err( "Retention block %s already has a name.", m->name );
				return -1;
			}

			m->name = str_perm( av->vptr, av->vlen );
		}
		else if( attIs( "match" ) )
		{
			__rkv_config_bkt_match_state = 1;

			if( !m->rgx )
				m->rgx = regex_list_create( 1 );

			return regex_list_add( av->vptr, 0, m->rgx );
		}
		else if( attIs( "fallbackMatch" ) )
		{
			__rkv_config_bkt_match_state = 1;

			if( !m->rgx )
				m->rgx = regex_list_create( 1 );

			return regex_list_add( av->vptr, 1, m->rgx );
		}
		else if( attIs( "default" ) )
		{
			__rkv_config_bkt_match_state = 1;
			m->is_default = config_bool( av );

			if( m->is_default && _rkv->ret_default )
			{
				err( "Cannot have two default retention blocks - %s is already set as deault.", _rkv->ret_default->name );
				return -1;
			}
		}
		else if( attIs( "done" ) )
		{
			if( !m->name )
			{
				m->name = mem_perm( 24 );
				snprintf( m->name, 24, "retention block %d", _rkv->rblocks );
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
			n->next = _rkv->retentions;
			_rkv->retentions = n;
			++(_rkv->rblocks);

			if( n->is_default )
				_rkv->ret_default = n;

			__rkv_config_bkt_match_state = 0;
		}
		else
			return -1;
	}

	return 0;
}
