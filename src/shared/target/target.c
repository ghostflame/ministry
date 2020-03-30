/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* target/target.c - handles network targets                               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


void target_write_all( IOBUF *buf )
{
	TGTL *l;

	for( l = _tgt->lists; l; l = l->next )
		io_buf_post( l, buf );
}


void target_add_metrics( TGT *t )
{
	TGTMT *m = _tgt->metrics;
	WORDS w;

	w.wd[0] = "target";
	w.wd[1] = t->name;
	w.wd[2] = "tgttype";
	w.wd[3] = t->typestr;

	// happens in ministry-test
	if( !w.wd[3] )
		w.wd[3] = "ministry";

	w.wc = 4;

	t->pm_bytes = pmet_create_gen( m->bytes, m->source, PMET_GEN_IVAL, &(t->bytes), NULL, NULL );
	pmet_label_apply_item( pmet_label_words( &w ), t->pm_bytes );

	t->pm_conn = pmet_create_gen( m->conn, m->source, PMET_GEN_IVAL, &(t->sock->connected), NULL, NULL );
	pmet_label_apply_item( pmet_label_words( &w ), t->pm_conn );
}


void target_loop( THRD *th )
{
	IO_CTL *io = _proc->io;
	struct sockaddr_in sa;
	int64_t fires = 0, r;
	struct timespec ts;
	TGT *t;

	t = (TGT *) th->arg;

	if( flagf_has( t, TGT_FLAG_STDOUT ) )
	{
		t->iofp = io_send_stdout;
	}
	else
	{
		if( net_lookup_host( t->host, &sa ) )
		{
			loop_end( "Unable to look up network target." );
			return;
		}

		if( t->proto == TARGET_PROTO_UDP )
			t->iofp = io_send_net_udp;
		else if( flagf_has( t, TGT_FLAG_TLS ) )
			t->iofp = io_send_net_tls;
		else
			t->iofp = io_send_net_tcp;
	}

	// we should already have a port
	sa.sin_port = htons( t->port );

	// make a socket with no buffers of its own
	t->sock = io_make_sock( 0, 0, &sa, t->flags, t->host );

	// calculate how many sleeps for reconnect
	r = 1000 * io->rc_msec;
	t->rc_limit = r / io->send_usec;
	if( r % io->send_usec )
		++(t->rc_limit);

	// make sure we have a non-zero max
	if( t->max == 0 )
		t->max = IO_MAX_WAITING;

	// make the queue - with a lock, but no free
	// callback - we add and remove things
	// explicitly
	t->queue = mem_list_create( 1, NULL );

	// and add some watcher metrics
	if( !runf_has( RUN_NO_HTTP ) )
	{
		target_add_metrics( t );
	}

	tgdebug( "Started target, max waiting %d", t->max );

	llts( ( 1000 * io->send_usec ), ts );

	loop_mark_start( "io" );

	// now loop around sending
	while( RUNNING( ) )
	{
		nanosleep( &ts, NULL );

		// call the io_fn
		fires += (*(t->iofp))( t );
	}

	loop_mark_done( "io", 0, fires );

	// disconnect
	io_disconnect( t->sock, 1 );
}



int target_run_one( TGT *t, int idx )
{
	target_set_id( t );

	// start a loop for each one
	thread_throw_named_f( target_loop, t, idx, "target_loop_%d", idx );
	return 0;
}


int target_run_list( TGTL *list )
{
	TGT *t;
	int i;

	if( !list )
		return -1;

	for( i = 0, t = list->targets; t; t = t->next, ++i )
		target_run_one( t, i );

	return 0;
}


int target_run( void )
{
	TGTL *l;

	if( !_tgt->lists )
		return -1;

	// check each one
	for( l = _tgt->lists; l; l = l->next )
		target_run_list( l );

	return 0;
}


TGTL *target_list_find( char *name )
{
	TGTL *l;

	if( name && *name )
		for( l = _tgt->lists; l; l = l->next )
			if( !strcasecmp( name, l->name ) )
				return l;

	return NULL;
}

TGT *target_list_search( TGTL *l, char *name, int len )
{
	TGT *t;

	if( !l || !name || !*name )
		return NULL;

	if( !len )
		len = strlen( name );

	for( t = l->targets; t; t = t->next )
		if( len == t->nlen && !strncasecmp( name, t->name, len ) )
			return t;

	return NULL;
}




