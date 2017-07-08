/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* target.c - functions to write to different backend targets              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry_test.h"


// attach a buffer for sending
// this is called from multiple threads
// they should never fight over a buffer
// but WILL fight over targets
//
// After a call to this function, the buffer
// is 'owned' by the target and should not be
// written to.  It will get recycled after
// sending
void target_buf_send( TARGET *s, IOBUF *buf )
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
	debug( "Target %s buf count is now %d", s->name, s->iolist->bufs );
}


const char *target_type_names[METRIC_TYPE_MAX] = 
{
	"adder", "stats", "gauge", "compat"
};

int target_start_one( TARGET *t )
{
	struct sockaddr_in sa;
	char namebuf[1024];
	int l;

	if( !t->active )
		return 0;

	// don't try to look up -
	if( t->to_stdout )
	{
		memset( &sa, 0, sizeof( struct sockaddr_in ) );
		l = snprintf( namebuf, 1024, "%s - stdout", t->label );
		t->iofp = &io_send_stdout;
	}
	else if( net_lookup_host( t->host, &sa ) )
	{
		err( "Cannot look up host %s -- invalid target.", t->host );
		return -1;
	}
	else
	{
		l = snprintf( namebuf, 1024, "%s - %s:%hu", t->label, t->host, t->port );
		t->iofp = &io_send_net;
	}

	// create its name
	t->name = str_dup( namebuf, l );

	// grab the rest of those params
	sa.sin_port   = htons( t->port );
	sa.sin_family = AF_INET;

	// make a socket
	t->sock = net_make_sock( 0, 0, &sa );

	// just connect to stdout?
	if( t->to_stdout )
		t->sock->sock = fileno( stdout );

	// how long do we count down after 
	t->reconn_ct = t->rc_usec / t->io_usec;
	if( t->rc_usec % t->io_usec )
		t->reconn_ct++;

	// run an io loop
	thread_throw( io_loop, t, 0 );

	return 0;
}


int target_start( void )
{
	TGT_CTL *c = ctl->tgt;
	int ret = 0;

	ret += target_start_one( c->stats );
	ret += target_start_one( c->adder );
	ret += target_start_one( c->gauge );
	ret += target_start_one( c->compat );

	return ret;
}




TARGET *__target_config_one( int8_t type, uint16_t port, char *str )
{
	TARGET *t;

	t          = (TARGET *) allocz( sizeof( TARGET ) );
	t->iolist  = mem_new_iolist( );
	t->label   = str_dup( str, 0 );
	t->port    = port;
	t->host    = strdup( "127.0.0.1" );
	t->type    = type;

	t->io_usec = 1000 * NET_IO_MSEC;
	t->rc_usec = 1000 * NET_RECONN_MSEC;

	pthread_mutex_init( &(t->iolist->lock), NULL );

	return t;
}


TGT_CTL *target_config_defaults( void )
{
	TGT_CTL *t;

	t = (TGT_CTL *) allocz( sizeof( TGT_CTL ) );

	t->adder  = __target_config_one( METRIC_TYPE_ADDER,  DEFAULT_ADDER_PORT,  "adder" );
	t->stats  = __target_config_one( METRIC_TYPE_STATS,  DEFAULT_STATS_PORT,  "stats" );
	t->gauge  = __target_config_one( METRIC_TYPE_GAUGE,  DEFAULT_GAUGE_PORT,  "gauge" );
	t->compat = __target_config_one( METRIC_TYPE_COMPAT, DEFAULT_COMPAT_PORT, "compat" );

	return t;
}





int target_config_line( AVP *av )
{
	TARGET *t;
	char *d;

	if( !( d = memchr( av->att, '.', av->alen ) ) )
		return -1;
	*d++ = '\0';

	if( attIs( "adder" ) )
		t = ctl->tgt->adder;
	else if( attIs( "stats" ) )
		t = ctl->tgt->stats;
	else if( attIs( "gauge" ) )
		t = ctl->tgt->gauge;
	else if( attIs( "compat" ) )
		t = ctl->tgt->compat;
	else
	{
		err( "Unrecognised target type: %s", av->att );
		return -1;
	}

	t->active = 1;

	if( !strcasecmp( d, "host" ) )
	{
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

		if( t->host )
			free( t->host );

		t->host = dup_val( );
	}
	else if( !strcasecmp( d, "port" ) )
	{
		t->port = (uint16_t) strtoul( av->val, NULL, 10 );
	}
	else if( !strcasecmp( d, "max_waiting" ) || !strcasecmp( d, "max" ) )
	{
		t->max = atoi( av->val );
		if( t->max <= 0 || t->max > TARGET_MAX_MAX_WAITING )
		{
			err( "Max waiting on a target must be 0 < max < %d", TARGET_MAX_MAX_WAITING );
			return -1;
		}
	}
	else if( !strcasecmp( d, "reconnect" ) || !strcasecmp( d, "reconnect_msec" ) )
	{
		t->rc_usec = 1000 * (uint32_t) strtoul( av->val, NULL, 10 );
	}
	else if( !strcasecmp( d, "io_msec" ) )
	{
		t->io_usec = 1000 * (uint32_t) strtoul( av->val, NULL, 10 );
	}
	else
		return -1;

	return 0;
}

