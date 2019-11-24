/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* io/senders.c - network i/o send control functions                       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



// io senders


// to the terminal|pipe
int64_t io_send_stdout( TGT *t )
{
	int64_t b = 0, f = 0;
	SOCK *s = t->sock;

	if( !s->out )
		io_buf_next( t );

	// keep writing while there's something to send
	while( s->out )
	{
		if( t->enabled )
		{
			b = write( s->fd, s->out->bf->buf + t->curr_off, s->out->bf->len - t->curr_off );
			t->curr_off += b;
			t->bytes += b;
			++f;
		}
		else
		{
			// just step over the buffer
			t->curr_off = t->curr_len;
		}

		if( t->curr_off >= t->curr_len )
		{
			io_buf_decr( s->out );
			io_buf_next( t );
		}
	}

	return f;
}


// to a network - needs to check connection
int64_t io_send_net_tcp( TGT *t )
{
	int64_t b = 0, f = 0;
	SOCK *s = t->sock;

	// are we waiting to reconnect?
	if( t->rc_count > 0 )
	{
		t->rc_count--;
		return 0;
	}

	// are we connected? or enabled?
	if( !t->enabled
	 || ( io_connected( s ) < 0
	   && io_connect( s ) < 0 ) )
	{
		t->rc_count = t->rc_limit;
		return 0;
	}

	if( !s->out )
		io_buf_next( t );

	while( s->out )
	{
		b = io_write_data( s, t->curr_off );
		t->curr_off += b;
		t->bytes += b;
		++f;

		// any problems?
		if( flagf_has( s, IO_CLOSE ) )
		{
			tgdebug( "Disconnecting from %s.", "target" );	// needs at least one arg :/
			io_disconnect( s );
			flagf_rmv( s, IO_CLOSE );
			// try again later
			break;
		}

		// did we send it all?
		if( t->curr_off >= t->curr_len )
		{
			// get a new one
			io_buf_decr( s->out );
			io_buf_next( t );
		}
		else
		{
			// try again later
			break;
		}
	}

	return f;
}


// not yet
int64_t io_send_net_udp( TGT *t )
{
	return -1;
}



// not yet
int64_t io_send_file( TGT *t )
{
	return -1;
}



