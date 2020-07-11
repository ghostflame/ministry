/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* rkv/init.c - start up our archive section                               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


int rkv_init( void )
{
	BUF *fbuf, *pbuf;
	int64_t tsa, fc;
	char idbuf[16];
	double diff;
	RKMET *me;
	RKBMT *m;
	int8_t i;
	WORDS w;
	RKFT *f;

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

	if( fs_mkdir_recursive( _rkv->base_path->buf ) != 0 )
	{
		err( "Could not find file base path %s -- %s",
			_rkv->base_path->buf, Err );
		return -1;
	}

	// create the tree hash
	_rkv->hash = string_store_create( _rkv->hashsz, NULL, 0, 1 );

	// make sure we don't make crazy paths allowed
	_rkv->maxpath = 2047 - RKV_EXTENSION_LEN - _rkv->base_path->len;

	// scan existing files
	fbuf = strbuf( _rkv->maxpath );
	pbuf = strbuf( _rkv->maxpath );
	strbuf_append( fbuf, _rkv->base_path );

	// scan our files, work out how long it took
	tsa = get_time64( );

	// how many files did we find?
	fc = rkv_scan_dir( fbuf, pbuf, 0, _rkv->root );

	// and get our timings
	diff = (double) ( get_time64( ) - tsa );
	diff /= BILLIONF;

	info( "Scanned %ld files in %.6f sec.", fc, diff );

	w.wc    = 2;
	w.wd[0] = "id";
	w.wd[1] = idbuf;

	me = _rkv->metrics;

	// and start our writers
	// needs moving back into archivist, for the tree hash
	for( i = 0; i < _rkv->wr_threads; ++i )
	{
		f = mem_perm( sizeof( RKFT ) );
		f->id = i;

		snprintf( idbuf, 16, "%d", i );

		f->updates   = pmet_create_gen( me->updates,   me->source, PMET_GEN_IVAL, &(f->up_ops),  NULL, NULL );
		f->pass_time = pmet_create_gen( me->pass_time, me->source, PMET_GEN_DVAL, &(f->up_time), NULL, NULL );

		pmet_label_apply_item( pmet_label_words( &w ), f->updates   );
		pmet_label_apply_item( pmet_label_words( &w ), f->pass_time );

		thread_throw_named_f( &rkv_writer, f, i, "rkv_writer_%d", i );
	}

	return 0;
}


