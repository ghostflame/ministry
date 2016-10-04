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

	if( b->proc.total == 0 )
		s->target->proc.total = 0;
	else
		s->target->proc.total = ( a->proc.total * s->factor ) / b->proc.total;
}

void synth_max( SYNTH *s )
{
	int i;

	s->target->proc.total = s->dhash[0]->proc.total;

	for( i = 1; i < s->parts; i++ )
		if( s->target->proc.total < s->dhash[i]->proc.total )
			s->target->proc.total = s->dhash[i]->proc.total;

	s->target->proc.total *= s->factor;
}

void synth_min( SYNTH *s )
{
	int i;

	s->target->proc.total = s->dhash[0]->proc.total;

	for( i = 1; i < s->parts; i++ )
		if( s->target->proc.total > s->dhash[i]->proc.total )
			s->target->proc.total = s->dhash[i]->proc.total;

	s->target->proc.total *= s->factor;
}

void synth_spread( SYNTH *s )
{
	double min, max;
	int i;

	min = max = s->dhash[0]->proc.total;

	for( i = 1; i < s->parts; i++ )
	{
		if( max < s->dhash[i]->proc.total )
			max = s->dhash[i]->proc.total;
		if( min > s->dhash[i]->proc.total )
			min = s->dhash[i]->proc.total;
	}

	s->target->proc.total = s->factor * ( max - min );
}

void synth_mean( SYNTH *s )
{
	synth_sum( s );
	s->target->proc.total /= s->parts;
}

void synth_count( SYNTH *s )
{
	int i;

	s->target->proc.total = 0;

	for( i = 0; i < s->parts; i++ )
		if( s->dhash[i]->proc.count > 0 )
			s->target->proc.total += 1;

	s->target->proc.total *= s->factor;
}

void synth_active( SYNTH *s )
{
	int i;

	s->target->proc.total = 0;

	for( i = 0; i < s->parts; i++ )
		if( s->dhash[i]->proc.count > 0 )
		{
			s->target->proc.total = 1;
			break;
		}
}



void synth_generate( SYNTH *s )
{
	uint64_t pt;
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
			if( ( s->dhash[i] = data_locate( s->paths[i], s->plens[i], DATA_TYPE_ADDER ) ) != NULL )
			{
				// exempt that from gc
				s->dhash[i]->empty = -1;
				s->missing--;
				debug_synth( "Found source %s for synth %s", s->dhash[i]->path, s->target_path );
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
		// check to see if there's any data
		for( pt = 0, i = 0; i < s->parts; i++ )
			pt += s->dhash[i]->proc.count;

		// make the point appropriately
		s->target->proc.count = pt;

		// only generate if there's anything to do
		if( pt > 0 )
		{
			debug_synth( "Generating synthetic %s", s->target_path );
			(s->fn)( s );
		}
	}
}




void synth_pass( int64_t tval, void *arg )
{
	ST_THR *t;
	SYNTH *s;

	debug_synth( "Attempting to get adder thread locks." );

	// try to get the adder locks
	// this should block until they are ready
	for( t = ctl->stats->adder->ctls; t; t = t->next )
		lock_stthr( t );

	debug_synth( "Generating synthetics." );

	// run the list
	for( s = ctl->synth->list; s; s = s->next )
		synth_generate( s );

	debug_synth( "Unlocking synth." );

	// unlock ourself
	unlock_synth( );

	debug_synth( "Unlocking adder thread locks." );

	// release those locks so the adder threads can carry on
	for( t = ctl->stats->adder->ctls; t; t = t->next )
		unlock_stthr( t );

	debug_synth( "Synth sleeping %d usec.", ctl->synth->wait_usec );

	// we have to make sure the adder threads have all called lock_synth before
	// this does - so we deliberately yield a little while
	// TODO device a more robust mechanism - another lock?
	usleep( ctl->synth->wait_usec );

	debug_synth( "Relocking synth." );

	// and relock ourself for next time
	lock_synth( );

	debug_synth( "Synth relocked after pass." );
}



void *synth_loop( void *arg )
{
	// a tenth of the adder period
	ctl->synth->wait_usec = ctl->stats->adder->period / 10;

	lock_synth( );

	// and loop
	loop_control( "synthetics", synth_pass, NULL, ctl->stats->adder->period, LOOP_SYNC, ctl->stats->adder->offset );

	// and lock ourself
	unlock_synth( );

	free( (THRD *) arg );
	return NULL;
}




void synth_init( void )
{
	SYNTH *s;
	int l;

	// reverse the list to preserve the order in config
	// this means synths can reference each other!
	ctl->synth->list = (SYNTH *) mtype_reverse_list( ctl->synth->list );

	// then light them up
	for( s = ctl->synth->list; s; s = s->next )
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

		info( "Synthetic '%s' has hash id %u.", s->target_path, s->target->id );
	}
}




SYN_CTL *synth_config_defaults( void )
{
	return (SYN_CTL *) allocz( sizeof( SYN_CTL ) );
}


static SYNTH __synth_cfg_tmp;
static int __synth_cfg_state = 0;


struct synth_fn_def synth_fn_defs[] =
{
	// the first name is the 'proper' name
	{ { "sum",    "add",     "plus" }, synth_sum,    1, 0 },
	{ { "diff",   "minus",   NULL   }, synth_diff,   2, 2 },
	{ { "ratio",  "div",     NULL   }, synth_div,    2, 2 },
	{ { "max",    "highest", NULL   }, synth_max,    1, 0 },
	{ { "min",    "lowest",  NULL   }, synth_min,    1, 0 },
	{ { "spread", "width",   NULL   }, synth_spread, 1, 0 },
	{ { "mean",   "average", NULL   }, synth_mean,   1, 0 },
	{ { "count",  "nonzero", NULL   }, synth_count,  1, 0 },
	{ { "active", "present", NULL   }, synth_active, 1, 0 },
	// last entry is a marker
	{ { NULL,     NULL,      NULL   }, NULL,         0, 0 },
};


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
	struct synth_fn_def *def;
	int i;

	// empty?
	if( !__synth_cfg_state )
	{
		memset( s, 0, sizeof( SYNTH ) );
		s->factor = 1;
		s->enable = 1;
		s->min_parts = 1;
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
	else if( attIs( "enable" ) )
	{
		s->enable = config_bool( av );
		__synth_cfg_state = 1;
	}
	else if( attIs( "factor" ) )
	{
		s->factor = strtod( av->val, NULL );
		__synth_cfg_state = 1;
	}
	else if( attIs( "operation" ) )
	{
		for( def = synth_fn_defs; def->fn; def++ )
		{
			if( valIs( def->names[0] )
			 || ( def->names[1] && valIs( def->names[1] ) )
			 || ( def->names[2] && valIs( def->names[2] ) ) )
			{
				s->fn        = def->fn;
				s->min_parts = def->min_parts;
				s->max_parts = def->max_parts;
				s->op_name   = def->names[0];
				break;
			}
		}

		if( !def )
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
			err( "Synthetic %s does not have enough sources (needs %d).",
				s->target_path, s->min_parts );
			return -1;
		}

		// will we have unused sources?
		if( s->max_parts > 0 && s->parts > s->max_parts )
		{
			warn( "Synthetic %s has %d sources but only %d will be used!",
				s->target_path, s->parts, s->max_parts );
		}

		if( s->enable )
		{
			// make a new synth and copy the contents
			// yes we are copying the pointers
			ns = (SYNTH *) allocz( sizeof( SYNTH ) );
			*ns = *s;

			// and link it
			ns->next = ctl->synth->list;
			ctl->synth->list = ns;
			ctl->synth->scount++;

			debug( "Added synthetic %s (%s/%d).", ns->target_path,
				ns->op_name, ns->parts );
		}
		else
		{
			// tidy up
			if( s->target_path )
			{
				debug( "Dropping disabled synthetic %s (%s/%d).",
					s->target_path, s->op_name, s->parts );

				free( s->target_path );
				s->target_path = NULL;
			}

			for( i = 0; i < s->parts; i++ )
				if( s->paths[i] )
				{
					free( s->paths[i] );
					s->paths[i] = NULL;
				}
		}

		// and we start again next time
		__synth_cfg_state = 0;
	}

	return 0;
}


