/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* synth/synth.c - synthetic metrics functions and config                  *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



void synth_generate( SYNTH *s )
{
	uint64_t pt, ct;
	int i;

	// check everything it needs exists...
	if( s->missing > 0 )
	{
		// some missing - go looking
		for( i = 0; i < s->parts; ++i )
		{
			if( s->dhash[i] )
				continue;

			// did we fetch a thing?
			if( ( s->dhash[i] = data_locate( s->paths[i], s->plens[i], DATA_TYPE_ADDER ) ) != NULL )
			{
				// exempt that from gc
				s->dhash[i]->empty = -1;
				s->missing--;
				debug( "Found source %s for synth %s", s->dhash[i]->path, s->target_path );
			}
		}

		// weirdness check
		if( s->missing < 0 )
		{
			warn( "Synthetic %s has missing count %d", s->target_path, s->missing );
			s->missing = 0;
		}
	}

	// are we a go?
	if( s->missing == 0 )
	{
		s->absent = 0;

		// check to see if there's any data
		for( pt = 0, i = 0; i < s->parts; ++i )
		{
			if( ( ct = s->dhash[i]->proc.count ) == 0 )
				++(s->absent);
			else
				pt += ct;
		}

		// make the point appropriately
		s->target->proc.count = pt;

		// make sure not too many are missing
		if( pt > 0 && s->absent <= s->max_absent )
		{
			// only generate if there's anything to do
			(s->def->fn)( s );

			// and mark it for reporting
			s->target->do_pass = 1;
		}
	}
}


void synth_pass( int64_t tval, void *arg )
{
	SYNTH *s;

	lock_synth( );

	while( _syn->tready < _syn->tcount )
	{
		pthread_cond_wait( &(_syn->threads_ready), &(ctl->locks->synth) );
	}

	// we are a go

	_syn->tready = 0;
	//debug( "Generating synthetics." );

	for( s = _syn->list; s; s = s->next )
		synth_generate( s );

	// tell everyone to proceed
	_syn->tproceed = _syn->tcount;

	pthread_cond_broadcast( &(_syn->threads_done) );

	// and release them
	unlock_synth( );
}




void synth_loop( THRD *t )
{
	// locking
	_syn->tcount = ctl->stats->adder->threads;
	pthread_cond_init( &(_syn->threads_ready), NULL );
	pthread_cond_init( &(_syn->threads_done),  NULL );

	// and loop
	loop_control( "synthetics", synth_pass, NULL, ctl->stats->adder->period, LOOP_SYNC, ctl->stats->adder->offset );

	pthread_cond_destroy( &(_syn->threads_ready) );
	pthread_cond_destroy( &(_syn->threads_done)  );
}




void synth_init( void )
{
	SYNTH *s;
	int l;

	// reverse the list to preserve the order in config
	// this means synths can reference each other!
	_syn->list = (SYNTH *) mem_reverse_list( _syn->list );

	// then light them up
	for( s = _syn->list; s; s = s->next )
	{
		l = strlen( s->target_path );

		// create the data point with 0 value
		data_point_adder( s->target_path, l, "0" );

		// find that dhash
		s->target = data_locate( s->target_path, l, DATA_TYPE_ADDER );

		// exempt it from gc
		s->target->empty = -1;

		// mark us as not having everything yet
		s->missing = s->parts;

		// and set the max absent; -ve values are parts - val
		if( s->max_absent < 0 )
		{
			if( s->def->max_absent >= 0 )
				s->max_absent = s->def->max_absent;
			else
				s->max_absent = s->parts + s->def->max_absent;
		}

		info( "Synthetic '%s' created.", s->target_path );
	}
}


