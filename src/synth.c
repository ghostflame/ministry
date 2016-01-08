/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* synth.c - synthetic metrics functions and config                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"

void synth_sum( SYNTH *s )
{
	int i;

	s->target->proc.total = 0;

	for( i = 0; i < s->parts; i++ )
		s->target->proc.total += s->dhash[i]->proc.total;

	s->target->proc.total *= s->factor;
}

void synth_diff( SYNTH *s )
{
	s->target->proc.total = s->factor * ( s->dhash[0]->proc.total - s->dhash[1]->proc.total );
}

void synth_div( SYNTH *s )
{
	DHASH *a, *b;

	a = s->dhash[0];
	b = s->dhash[1];

	// UGH, this is integer maths
	s->target->proc.total = ( b->proc.total > 0 ) ? ( s->factor * a->proc.total ) / b->proc.total : 0;
}

void synth_max( SYNTH *s )
{
	int i;

	s->target->proc.total = s->dhash[0]->proc.total;

	for( i = 1; i < s->parts; i++ )
		if( s->dhash[i]->proc.total > s->target->proc.total )
			s->target->proc.total = s->dhash[i]->proc.total;
}

void synth_min( SYNTH *s )
{
	int i;

	s->target->proc.total = s->dhash[0]->proc.total;

	for( i = 1; i < s->parts; i++ )
		if( s->dhash[i]->proc.total < s->target->proc.total )
			s->target->proc.total = s->dhash[i]->proc.total;
}




void synth_generate( SYNTH *s )
{
	int i;

	// check everything it needs exists...
	if( s->missing > 0 )
	{
		// some missing - go looking
		for( i = 0; i < s->parts; i++ )
		{
			if( s->dhash[i] )
				continue;

			// did we fetch a thing?
			if( ( s->dhash[i] = data_locate( s->paths[i], s->plens[i], DATA_HTYPE_ADDER ) ) != NULL )
			{
				// exempt that from gc
				s->dhash[i]->empty = -1;
				s->missing--;
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
		(s->fn)( s );
}




void synth_pass( uint64_t tval, void *arg )
{
	ST_THR *t;
	SYNTH *s;

	//debug( "Attempting to get adder thread locks." );

	// try to get the adder locks
	// this should block until they are ready
	for( t = ctl->stats->adder->ctls; t; t = t->next )
		lock_stthr( t );

	//debug( "Generating synthetics." );

	// run the list
	for( s = ctl->synth->list; s; s = s->next )
		synth_generate( s );

	//debug( "Unlocking synth." );

	// unlock ourself
	unlock_synth( );

	//debug( "Unlocking adder thread locks." );

	// release those locks so the adder threads can carry on
	for( t = ctl->stats->adder->ctls; t; t = t->next )
		unlock_stthr( t );

	//debug( "Sleeping %d usec.", ctl->synth->wait_usec );

	// we have to make sure the adder threads have all called lock_synth before
	// this does - so we deliberately yield a little while
	// TODO device a more robust mechanism - another lock?
	usleep( ctl->synth->wait_usec );

	//debug( "Relocking synth." );

	// and relock ourself for next time
	lock_synth( );
}



void *synth_loop( void *arg )
{
	THRD *t = (THRD *) arg;

	// a tenth of the adder period
	ctl->synth->wait_usec = ctl->stats->adder->period / 10;

	lock_synth( );

	// and loop
	loop_control( "synthetics", synth_pass, NULL, ctl->stats->adder->period , 1, ctl->stats->adder->offset );

	// and lock ourself
	unlock_synth( );

	free( t );
	return NULL;
}




void synth_init( void )
{
	SYNTH *s, *list;
	int l;

	// reverse the list to preserve the order in config
	// this means synths can reference each other!
	list = ctl->synth->list;
	ctl->synth->list = NULL;

	while( list )
	{
		s = list;
		list = s->next;

		s->next = ctl->synth->list;
		ctl->synth->list = s;
	}


	// then light them up
	for( s = ctl->synth->list; s; s = s->next )
	{
		l = strlen( s->target_path );

		// create the data point with 0 value
		data_point_adder( s->target_path, l, 0 );

		// find that dhash
		s->target = data_locate( s->target_path, l, DATA_HTYPE_ADDER );

		// exempt it from gc
		s->target->empty = -1;

		// mark us as not having everything yet
		s->missing = s->parts;

		info( "Synthetic '%s' has hash id %u", s->target_path, s->target->id );
	}
}




SYN_CTL *synth_config_defaults( void )
{
	return (SYN_CTL *) allocz( sizeof( SYN_CTL ) );
}


static SYNTH __synth_cfg_tmp;
static int __synth_cfg_state = 0;

int synth_config_path( SYNTH *s, AVP *av )
{
	char *p;

	if( s->target_path )
		p = s->target_path;
	else
		p = "unknown";

	if( s->parts == SYNTH_PART_MAX )
	{
		warn( "Cannot add another source to synthetic %s", p );
		return -1;
	}

	s->paths[s->parts] = str_copy( av->val, av->vlen );
	s->plens[s->parts] = av->vlen;
	s->parts++;

	return 0;
}


int synth_config_line( AVP *av )
{
	SYNTH *ns, *s = &__synth_cfg_tmp;

	// empty?
	if( !__synth_cfg_state )
	{
		memset( s, 0, sizeof( SYNTH ) );
		s->factor = 1;
	}

	if( attIs( "path" ) || attIs( "target" ) )
	{
		// clear existing
		if( s->target_path )
			free( s->target_path );

		s->target_path = str_copy( av->val, av->vlen );
		__synth_cfg_state = 1;
	}
	else if( attIs( "source" ) )
	{
		if( synth_config_path( s, av ) != 0 )
			return -1;
		__synth_cfg_state = 1;
	}
	else if( attIs( "factor" ) )
	{
		s->factor = strtoull( av->val, NULL, 10 );
		__synth_cfg_state = 1;
	}
	else if( attIs( "operation" ) )
	{
		if( valIs( "add" ) || valIs( "plus" ) || valIs( "sum" ) )
		{
			s->fn = synth_sum;
			s->min_parts = 1;
		}
		else if( valIs( "diff" ) || valIs( "minus" ) )
		{
			s->fn = synth_diff;
			s->min_parts = 2;
		}
		else if( valIs( "div" ) || valIs( "ratio" ) )
		{
			s->fn = synth_div;
			s->min_parts = 2;
		}
		else if( valIs( "max" ) || valIs( "greatest" ) )
		{
			s->fn = synth_max;
			s->min_parts = 1;
		}
		else if( valIs( "min" ) || valIs( "least" ) )
		{
			s->fn = synth_min;
			s->min_parts = 1;
		}
		else
		{
			err( "Synthetic operation %s not recognised.", av->val );
			return -1;
		}

		__synth_cfg_state = 1;
	}
	else if( attIs( "done" ) )
	{
		if( !__synth_cfg_state
		 || !s->fn
		 || !s->parts
		 || !s->target_path )
		{
			err( "Synthetic config is not complete!" );
			return -1;
		}

		if( s->parts < s->min_parts )
		{
			err( "Synthetic %s does not have enough sources (needs %d).", s->target_path, s->min_parts );
			return -1;
		}

		// make a new synth and copy the contents
		// yes we are copying the pointers
		ns = (SYNTH *) allocz( sizeof( SYNTH ) );
		*ns = *s;

		// and link it
		ns->next = ctl->synth->list;
		ctl->synth->list = ns;
		ctl->synth->scount++;

		__synth_cfg_state = 0;
	}

	return 0;
}


