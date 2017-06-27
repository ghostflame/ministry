/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* target.c - functions to write to different backend targets              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry_test.h"


// graphite format: <path> <value> <timestamp>\n
void target_write_graphite( ST_THR *t, BUF *ts, IOBUF *b )
{
	// copy the prefix
	memcpy( b->buf + b->len, t->prefix->buf, t->prefix->len );
	b->len += t->prefix->len;

	// then the path and value
	memcpy( b->buf + b->len, t->path->buf, t->path->len );
	b->len += t->path->len;

	// add the timestamp and newline
	memcpy( b->buf + b->len, ts->buf, ts->len );
	b->len += ts->len;
}



// tsdb, has to be different
// put <metric> <timestamp> <value> <tagk1=tagv1[ tagk2=tagv2 ...tagkN=tagvN]>
// we don't really have tagging :-(  so fake it
void target_write_opentsdb( ST_THR *t, BUF *ts, IOBUF *b )
{
	char *space;
	int k, l;

	if( !( space = memrchr( t->path->buf, ' ', t->path->len ) ) )
		return;

	k = space++ - t->path->buf;
	l = t->path->len - k - 1;

	// it starts with a put
	memcpy( b->buf + b->len, "put ", 4 );
	b->len += 4;

	// copy the prefix
	memcpy( b->buf + b->len, t->prefix->buf, t->prefix->len );
	b->len += t->prefix->len;

	// then just the path
	memcpy( b->buf + b->len, t->path->buf, k );
	b->len += k;

	// now the timestamp, but without the newline
	memcpy( b->buf + b->len, ts->buf, ts->len - 1 );
	b->len += ts->len - 1;

	// now another space
	b->buf[b->len++] = ' ';

	// then the value
	memcpy( b->buf + b->len, space, l );
	b->len += l;

	// then a space, a tag and a newline
	memcpy( b->buf + b->len, " source=ministry\n", 17 );
	b->len += 17;
}



// attach a buffer for sending
// this is called from multiple threads
// they should never fight over a buffer
// but WILL fight over target sets
//
// After a call to this function, the buffer
// is 'owned' by the tset and should not be
// written to.  It will get recycled after
// sending
void target_buf_send( TSET *s, IOBUF *buf )
{
	if( !buf )
		return;

	// it does contain something, right?
	if( !buf->len )
	{
		//debug( "Empty buffer passed to target_buf_send." );
		mem_free_buf( &buf );
		return;
	}

	io_post_buffer( s->iolist, buf );
	debug_io( "Tset %s buf count is now %d", s->stype->name, s->iolist->bufs );
}




void *target_set_loop( void *arg )
{
	TARGET *tg;
	TGTIO *io;
	IOBUF *b;
	TSET *s;
	THRD *t;
    int i;

	t = (THRD *) arg;
	s = (TSET *) t->arg;

	// throw the individual target loops
	for( i = 0, tg = s->targets; tg; tg = tg->next, i++ )
		thread_throw( io_loop, tg, i );

	// and start watching and posting buffers to targets
	while( ctl->run_flags & RUN_LOOP )
	{
		usleep( ctl->net->io_usec );

		while( ( b = io_fetch_buffer( s->iolist ) ) )
			for( tg = s->targets; tg; tg = tg->next )
			{
				io = tg->iolist;

				// set the buffer to be handled by each target
				// we have to do it here to prevent us getting
				// tripped up midway through handing it to the
				// io threads
				b->refs = s->count;

				// unless it's full already in which case
				// decrement and maybe free
				if( io->bufs > tg->max )
				{
					debug_io( "Dropping buffer to target %s", tg->name );
					io_decr_buf( b );
					continue;
				}

				// and post it
				debug_io( "Sending buffer %p to target %s", b, tg->name );
				io_post_buffer( io, b );
			}
	}

	free( t );
	return NULL;
}




int target_start( void )
{
	TGT_CTL *c = ctl->tgt;
	TSET *s;
	int i;

	if( !c->tgt_count )
	{
		fatal( "No targets configured." );
		return -1;
	}

	// and make the flat list of set pointers
	c->setarr = (TSET **) allocz( c->set_count * sizeof( TSET * ) );

	// start each target set
	// and make the flat list
	for( i = 0, s = c->sets; s; s = s->next, i++ )
	{
		c->setarr[i] = s;
		thread_throw( target_set_loop, s, i );
	}

	return 0;
}




TGT_CTL *target_config_defaults( void )
{
	TGT_CTL *t;
	TSTYPE *ts;
	TTYPE *tt;
	int i, j;

	// link up the types and stypes
	for( i = 0; i < TGT_TYPE_MAX; i++ )
	{
		tt = &(target_type_defns[i]);
		for( j = 0; j < TGT_STYPE_MAX; j++ )
		{
			ts = &(target_stype_defns[j]);
			if( tt->st_id == ts->stype )
			{
				tt->stype = ts;
				break;
			}
		}
	}

	t = (TGT_CTL *) allocz( sizeof( TGT_CTL ) );

	return t;
}



int target_add_config( TARGET *t )
{
	struct sockaddr_in sa;
	TGT_CTL *c = ctl->tgt;
	char namebuf[1024];
	TSET *s;
	int l;

	// don't try to look up '-'
	if( t->to_stdout )
	{
		memset( &sa, 0, sizeof( struct sockaddr_in ) );
		l = snprintf( namebuf, 1024, "%s - stdout", t->type->name );
		t->iofp = &io_send_stdout;
	}
	else if( net_lookup_host( t->host, &sa ) )
	{
		err( "Cannot look up host %s -- invalid target.", t->host );
		return -1;
	}
	else
	{
		l = snprintf( namebuf, 1024, "%s - %s:%hu", t->type->name, t->host, t->port );
		t->iofp = &io_send_net;
	}

	// create its name
	t->name = str_dup( namebuf, l );

	// make sure we have a target set to put it in
	for( s = c->sets; s; s = s->next )
		if( s->stype == t->type->stype )
			break;

	// we need a new one
	if( !s )
	{
		s = (TSET *) allocz( sizeof( TSET ) );
		s->stype  = t->type->stype;
		s->iolist = (TGTIO *) allocz( sizeof( TGTIO ) );

		pthread_mutex_init( &(s->lock), NULL );

		s->next = c->sets;
		c->sets = s;
		c->set_count++;
	}

	// grab the rest of those params
	sa.sin_port   = htons( t->port );
	sa.sin_family = AF_INET;

	// make a socket
	t->sock = net_make_sock( 0, 0, &sa );

	// just connect to stdout?
	if( t->to_stdout )
		t->sock->sock = fileno( stdout );

	// how long do we count down after 
	t->reconn_ct = ctl->net->reconn / ctl->net->io_usec;
	if( ctl->net->reconn % ctl->net->io_usec )
		t->reconn_ct++;

	// it's io list
	t->iolist = (TGTIO *) allocz( sizeof( TGTIO ) );

	// put it in the list
	t->next    = s->targets;
	s->targets = t;
	t->set     = s;

	// and keep count
	s->count++;
	c->tgt_count++;

	return 0;
}




static TARGET __tgt_cfg_tmp;
static int __tgt_cfg_state = 0;

int target_config_line( AVP *av )
{
	TARGET *nt, *t = &__tgt_cfg_tmp;
	TTYPE *tt;
	int i;

	// empty?
	if( !__tgt_cfg_state )
	{
		memset( t, 0, sizeof( TARGET ) );
		t->max = DEFAULT_MAX_WAITING;
	}

	if( attIs( "type" ) )
	{
		tt = &(target_type_defns[0]);
		for( i = 0; i < TGT_TYPE_MAX; i++, tt++ )
			if( valIs( tt->name ) )
			{
				t->type = tt;

				// fill it the default port?
				if( t->port == 0 )
					t->port = tt->port;

				__tgt_cfg_state = 1;
				return 0;
			}

		err( "Unrecognised target type: %s", av->val );
		return -1;
	}
	else if( attIs( "host" ) )
	{
		if( t->host )
		{
			err( "Target already has host %s", t->host );
			return -1;
		}

		if( strchr( av->val, ' ' ) )
		{
			err( "Target hosts may not have spaces in them (%s)", av->val );
			return -1;
		}

		// special value - stdout
		if( valIs( "-" ) )
		{
			if( ctl->run_flags & RUN_TGT_STDOUT )
			{
				err( "Can only have one target writing to stdout." );
				return -1;
			}

			ctl->run_flags |= RUN_TGT_STDOUT;
			t->to_stdout = 1;

			debug( "Target writing to stdout." );
		}

		t->host = str_dup( av->val, av->vlen );
		__tgt_cfg_state = 1;
	}
	else if( attIs( "port" ) )
	{
		t->port = (uint16_t) strtoul( av->val, NULL, 10 );
		__tgt_cfg_state = 1;
	}
	else if( attIs( "max_waiting" ) || attIs( "max" ) )
	{
		t->max = atoi( av->val );
		if( t->max <= 0 || t->max > TARGET_MAX_MAX_WAITING )
		{
			err( "Max waiting on a target must be 0 < max < %d", TARGET_MAX_MAX_WAITING );
			return -1;
		}
		__tgt_cfg_state = 1;
	}
	else if( attIs( "done" ) )
	{
		if( !t->type || !t->host || !t->port )
		{
			err( "Incomplete target spec: ( %s:%hu, type %s )",
				( t->host ) ? t->host : "'unknown host'",
				t->port,
				( t->type ) ? t->type->name : "'unknown type'" );
			return -1;
		}

		// copy it
		nt = (TARGET *) allocz( sizeof( TARGET ) );
		memcpy( nt, t, sizeof( TARGET ) );

		// ready for another
		__tgt_cfg_state = 0;

		// and add it
		return target_add_config( nt );
	}
	else
		return -1;

	return 0;
}

