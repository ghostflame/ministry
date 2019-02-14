/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* target.c - handles network targets                                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"

TGT_CTL *_tgt;


void target_loop( THRD *th )
{
	struct sockaddr_in sa;
	int64_t fires = 0, r;
	TGT *t;

	t = (TGT *) th->arg;

	if( t->to_stdout )
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
		else
			t->iofp = io_send_net_tcp;
	}

	// we should already have a port
	sa.sin_port = htons( t->port );

	// make a socket with no buffers of its own
	t->sock = io_make_sock( 0, 0, &sa );

	r = 1000 * _io->rc_msec;
	t->rc_limit = r / _io->send_usec;
	if( r % _io->send_usec )
		t->rc_limit++;

	// make sure we have a non-zero max
	if( t->max == 0 )
		t->max = IO_MAX_WAITING;

	// init the lock
	io_lock_init( t->lock );

	tgdebug( "Started target, max waiting %d", t->max );

	loop_mark_start( "io" );

	// now loop around sending
	while( RUNNING( ) )
	{
		usleep( _io->send_usec );

		// call the io_fn
		fires += (*(t->iofp))( t );
	}

	loop_mark_done( "io", 0, fires );

	// disconnect
	io_disconnect( t->sock );
	io_lock_destroy( t->lock );
}



int target_run_one( TGT *t, int enabled_check, int idx )
{
	if( enabled_check && !t->enabled )
	{
		info( "Ignoring target %s due to enabled check.", t->name );
		return 1;
	}

	target_set_id( t );

	// start a loop for each one
	thread_throw_named( target_loop, t, idx, "target_loop" );
	return 0;
}


int target_run_list( TGTL *list, int enabled_check )
{
	TGT *t;
	int i;

	if( !list )
		return -1;

	for( i = 0, t = list->targets; t; t = t->next, i++ )
		target_run_one( t, enabled_check, i );

	return 0;
}


int target_run( int enabled_check )
{
	TGTL *l;

	if( !_tgt->lists )
		return -1;

	// check each one
	for( l = _tgt->lists; l; l = l->next )
		target_run_list( l, enabled_check );

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





// config


void target_set_handle( TGT *t, char *prefix )
{
	char buf[1024];
	int l;

	if( !prefix )
		prefix = t->name;

	if( t->to_stdout )
		l = snprintf( buf, 1024, "%s - stdout", prefix );
	else
		l = snprintf( buf, 1024, "%s - %s:%hu", prefix, t->host, t->port );

	if( t->handle )
		free( t->handle );

	t->handle = str_copy( buf, l );
}



int __target_set_maxw( TGT *t, char *max )
{
	int64_t i;

	if( parse_number( max, &i, NULL ) == NUM_INVALID )
	{
		err( "Invalid maximum waiting buffers for target: %s", max );
		return -1;
	}

	if( i < 0 || i > IO_LIM_WAITING )
	{
		err( "Max waiting on a target must be 0 < max < %d", IO_LIM_WAITING );
		return -2;
	}

	t->max = (int32_t) i;
	return 0;
}


int __target_set_host( TGT *t, char *host )
{
	if( strchr( host, ' ' ) )
	{
		err( "Target hosts may not have spaces in them: %s", host );
		return -1;
	}

	// special value, stdout
	if( !strcmp( host, "-" ) )
	{
		if( _io->stdout_count > 0 )
		{
			err( "Can only have one target writing to stdout." );
			return -1;
		}

		t->to_stdout = 1;
		_io->stdout_count++;
		debug( "Target writing to stdout." );
	}

	t->host = str_dup( host, 0 );
	return 0;
}

int __target_set_protocol( TGT *t, char *proto )
{
	if( !strcasecmp( proto, "tcp" ) )
		t->proto = TARGET_PROTO_TCP;
	else if( !strcasecmp( proto, "udp" ) )
		t->proto = TARGET_PROTO_UDP;
	else
		return -1;

	return 0;
}

// find or make a new list
TGTL *__target_list_find_create( char *name )
{
	TGTL *l;

	if( !( l = target_list_find( name ) ) )
	{
		l           = (TGTL *) allocz( sizeof( TGTL ) );
		l->name     = str_dup( name, 0 );
		l->next     = _tgt->lists;
		_tgt->lists = l;
		_tgt->count++;
	}

	return l;
}

TGTL *target_list_create( char *name )
{
	return __target_list_find_create( name );
}


TGT *target_create( char *list, char *name, char *proto, char *host, uint16_t port, char *type, int enabled )
{
	TGT *t;

	t = (TGT *) allocz( sizeof( TGT ) );
	t->port = port;
	t->name = str_copy( name, 0 );
	t->list = __target_list_find_create( list );
	t->enabled = enabled;

	if( _tgt->type_fn )
		(*(_tgt->type_fn))( t, type, strlen( type ) );

	t->next = t->list->targets;
	t->list->targets = t;
	_tgt->tcount++;

	return t;
}

TGT *target_create_str( char *src, int len, char sep )
{
	WORDS w;

	if( !len )
		len = strlen( src );

	if( !sep )
		sep = ':';

	strwords( &w, src, len, sep );
	if( w.wc != 7 )
	{
		err( "Invalid target all-in-one source." );
		return NULL;
	}
	
	return target_create( w.wd[0], w.wd[1], w.wd[2], w.wd[3],
		(uint16_t) strtoul( w.wd[4], NULL, 10 ),
		w.wd[5], atoi( w.wd[6] ) );
}


void target_set_type_fn( target_cfg_fn *fp )
{
	_tgt->type_fn = fp;
}

void target_set_data_fn( target_cfg_fn *fp )
{
	_tgt->type_fn = fp;
}



TGT_CTL *target_config_defaults( void )
{
	_tgt = (TGT_CTL *) allocz( sizeof( TGT_CTL ) );
	return _tgt;
}


static TGT __tgt_cfg_tmp;
static int __tgt_cfg_state = 0;

int target_config_line( AVP *av )
{
	TGT *n, *p, *t = &__tgt_cfg_tmp;

	// empty?
	if( !__tgt_cfg_state )
	{
		memset( t, 0, sizeof( TGT ) );
		t->max = IO_MAX_WAITING;
	}

	if( attIs( "target" ) )
	{
		// all in one go
		if( !target_create_str( av->vptr, av->vlen, ':' ) )
			return -1;

		__tgt_cfg_state = 0;
	}
	else if( attIs( "list" ) )
	{
		t->list = __target_list_find_create( av->vptr );
		__tgt_cfg_state = 1;
	}
	else if( attIs( "name" ) )
	{
		if( t->name )
			free( t->name );

		t->name = str_copy( av->vptr, av->vlen );
		__tgt_cfg_state = 1;
	}
	else if( attIs( "host" ) )
	{
		if( __target_set_host( t, av->vptr ) )
			return -1;
		__tgt_cfg_state = 1;
	}
	else if( attIs( "port" ) )
	{
		t->port = (uint16_t) strtoul( av->vptr, NULL, 10 );
		__tgt_cfg_state = 1;
	}
	else if( attIs( "maxWaiting" ) || attIs( "max" ) )
	{
		if( __target_set_maxw( t, av->vptr ) )
			return -1;
		__tgt_cfg_state = 1;
	}
	else if( attIs( "enable" ) || attIs( "enabled" ) )
	{
		t->enabled = config_bool( av );
		__tgt_cfg_state = 1;
	}
	else if( attIs( "protocol" ) )
	{
		if( __target_set_protocol( t, av->vptr ) )
			return -1;
		__tgt_cfg_state = 1;
	}
	else if( attIs( "type" ) )
	{
		if( t->typestr )
		{
			warn( "Target already has a type set." );
			free( t->typestr );
		}
		t->typestr = str_copy( av->vptr, av->vlen );

		if( _tgt->type_fn )
		{
			if( (*(_tgt->type_fn))( t, av->vptr, av->vlen ) )
				return -1;
		}
		else
			warn( "Target type configured but no type callback set." );

		__tgt_cfg_state = 1;
	}
	else if( attIs( "data" ) )
	{
		if( _tgt->data_fn )
		{
			if( (*(_tgt->data_fn))( t, av->vptr, av->vlen ) )
				return -1;
		}
		else
			warn( "Target data configured but no data callback set." );

		__tgt_cfg_state = 1;
	}
	else if( attIs( "done" ) )
	{
		if( !t->host )
			__target_set_host( t, "127.0.0.1" );

		if( !t->proto )
			t->proto = TARGET_PROTO_TCP;

		if( !t->name )
		{
			char autonamebuf[256];
			int anbl;

			warn( "No target name provided." );
			if( t->typestr )
				anbl = snprintf( autonamebuf, 256, "%s:%s:%hu", t->typestr, t->host, t->port );
			else
				anbl = snprintf( autonamebuf, 256, "%s:%hu", t->host, t->port );

			t->name = str_copy( autonamebuf, anbl );
		}

		if( !t->list )
			t->list = __target_list_find_create( t->name );

		n = (TGT *) allocz( sizeof( TGT ) );
		memcpy( n, t, sizeof( TGT ) );

		// add it into the list - preserve order, so append
		if( !n->list->targets )
			n->list->targets = n;
		else
		{
			// run to the end (yes, trailing ; is intended)
			for( p = n->list->targets; p->next; p = p->next );

			p->next = n;
		}
		n->list->count++;
		_tgt->tcount++;

		__tgt_cfg_state = 0;
	}
	else
		return -1;

	return 0;
}

