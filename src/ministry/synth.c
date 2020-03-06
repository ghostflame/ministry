/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* synth.c - synthetic metrics functions and config                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"

static inline int __synth_set_first( SYNTH *s )
{
	int i;

	for( i = 0; i < s->parts; ++i )
		if( s->dhash[i]->proc.count > 0 )
		{
			s->target->proc.total = s->dhash[i]->proc.total;
			break;
		}

	return i + 1;
}


void synth_sum( SYNTH *s )
{
	int i;

	s->target->proc.total = 0;

	for( i = 0; i < s->parts; ++i )
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

void synth_prod( SYNTH *s )
{
	int i;

	s->target->proc.total = s->factor;

	// only use present values
	for( i = 0; i < s->parts; ++i )
		if( s->dhash[i]->proc.count > 0 )
			s->target->proc.total *= s->dhash[i]->proc.total;
}

void synth_cap( SYNTH *s )
{
	DHASH *a, *b;

	a = s->dhash[0];
	b = s->dhash[1];

	s->target->proc.total = ( a->proc.total < b->proc.total ) ? a->proc.total : b->proc.total;
}

void synth_max( SYNTH *s )
{
	int i;

	i = __synth_set_first( s );

	for( ; i < s->parts; ++i )
		if( s->target->proc.total < s->dhash[i]->proc.total )
			s->target->proc.total = s->dhash[i]->proc.total;

	s->target->proc.total *= s->factor;
}

void synth_min( SYNTH *s )
{
	int i;

	i = __synth_set_first( s );

	for( ; i < s->parts; ++i )
		if( s->target->proc.total > s->dhash[i]->proc.total )
			s->target->proc.total = s->dhash[i]->proc.total;

	s->target->proc.total *= s->factor;
}

void synth_spread( SYNTH *s )
{
	double min, max;
	int i;

	i = __synth_set_first( s );

	min = max = s->target->proc.total;

	for( ; i < s->parts; ++i )
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

void synth_meanIf( SYNTH *s )
{
	synth_sum( s );
	s->target->proc.total /= ( s->parts - s->absent );
}

void synth_count( SYNTH *s )
{
	s->target->proc.total = s->factor * ( s->parts - s->absent );
}

void synth_active( SYNTH *s )
{
	// we only get here if we had one active
	s->target->proc.total = 1;
}




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
	SYN_CTL *sc = ctl->synth;
	SYNTH *s;

	lock_synth( );

	while( sc->tready < sc->tcount )
	{
		pthread_cond_wait( &(sc->threads_ready), &(ctl->locks->synth) );
	}

	// we are a go

	sc->tready = 0;
	//debug( "Generating synthetics." );

	for( s = sc->list; s; s = s->next )
		synth_generate( s );

	// tell everyone to proceed
	sc->tproceed = sc->tcount;

	pthread_cond_broadcast( &(sc->threads_done) );

	// and release them
	unlock_synth( );
}




void synth_loop( THRD *t )
{
	SYN_CTL *sc = ctl->synth;

	// locking
	sc->tcount = ctl->stats->adder->threads;
	pthread_cond_init( &(sc->threads_ready), NULL );
	pthread_cond_init( &(sc->threads_done),  NULL );

	// and loop
	loop_control( "synthetics", synth_pass, NULL, ctl->stats->adder->period, LOOP_SYNC, ctl->stats->adder->offset );

	pthread_cond_destroy( &(sc->threads_ready) );
	pthread_cond_destroy( &(sc->threads_done)  );
}




void synth_init( void )
{
	SYNTH *s;
	int l;

	// reverse the list to preserve the order in config
	// this means synths can reference each other!
	ctl->synth->list = (SYNTH *) mem_reverse_list( ctl->synth->list );

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

		// and set the max absent; -ve values are parts - val
		if( s->max_absent < 0 )
		{
			if( s->def->max_absent >= 0 )
				s->max_absent = s->def->max_absent;
			else
				s->max_absent = s->parts + s->def->max_absent;
		}

		info( "Synthetic '%s' has hash id %u.", s->target_path, s->target->id );
	}
}




SYN_CTL *synth_config_defaults( void )
{
	return (SYN_CTL *) mem_perm( sizeof( SYN_CTL ) );
}


static SYNTH __synth_cfg_tmp;
static int __synth_cfg_state = 0;


struct synth_fn_def synth_fn_defs[] =
{
	// the first name is the 'proper' name
	{ { "sum",    "add",     "plus" }, synth_sum,    1, 0, SYNTH_PART_MAX },
	{ { "diff",   "minus",   NULL   }, synth_diff,   2, 2, 0 },
	{ { "ratio",  "div",     NULL   }, synth_div,    2, 2, 0 },
	{ { "mult",   "product", NULL   }, synth_prod,   1, 0, SYNTH_PART_MAX },
	{ { "cap",    "limit",   NULL   }, synth_cap,    2, 2, 0 },
	{ { "max",    "highest", NULL   }, synth_max,    2, 0, SYNTH_PART_MAX },
	{ { "min",    "lowest",  NULL   }, synth_min,    2, 0, SYNTH_PART_MAX },
	{ { "spread", "width",   NULL   }, synth_spread, 2, 0, -2 },
	{ { "mean",   "average", NULL   }, synth_mean,   2, 0, -1 },
	{ { "meanIf", "avgIf",   NULL   }, synth_meanIf, 2, 0, -1 },
	{ { "count",  "nonzero", NULL   }, synth_count,  1, 0, SYNTH_PART_MAX },
	{ { "active", "present", NULL   }, synth_active, 1, 0, SYNTH_PART_MAX },
	// last entry is a marker
	{ { NULL,     NULL,      NULL   }, NULL,         0, 0, SYNTH_PART_MAX },
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

	s->paths[s->parts] = str_copy( av->vptr, av->vlen );
	s->plens[s->parts] = av->vlen;

	++(s->parts);

	return 0;
}



int synth_config_line( AVP *av )
{
	SYNTH *ns, *s = &__synth_cfg_tmp;
	SYNDEF *def;
	int i;

	// empty?
	if( !__synth_cfg_state )
	{
		memset( s, 0, sizeof( SYNTH ) );
		s->factor = 1;
		s->enable = 1;
		s->max_absent = -1;
	}

	if( attIs( "path" ) || attIs( "target" ) )
	{
		// clear existing
		if( s->target_path )
			free( s->target_path );

		s->target_path = str_copy( av->vptr, av->vlen );
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
		s->factor = strtod( av->vptr, NULL );
		__synth_cfg_state = 1;
	}
	else if( attIs( "maxAbsent" ) )
	{
		s->max_absent = strtol( av->vptr, NULL, 10 );
		__synth_cfg_state = 1;
	}
	else if( attIs( "operation" ) )
	{
		for( def = synth_fn_defs; def->fn; ++def )
		{
			if( valIs( def->names[0] )
			 || ( def->names[1] && valIs( def->names[1] ) )
			 || ( def->names[2] && valIs( def->names[2] ) ) )
			{
				s->def = def;
				break;
			}
		}

		if( !def )
		{
			err( "Synthetic operation %s not recognised.", av->vptr );
			return -1;
		}

		__synth_cfg_state = 1;
	}
	else if( attIs( "done" ) )
	{
		if( !__synth_cfg_state
		 || !s->def
		 || !s->parts
		 || !s->target_path )
		{
			err( "Synthetic config is not complete!" );
			return -1;
		}

		if( s->parts < s->def->min_parts )
		{
			err( "Synthetic %s does not have enough sources (needs %d).",
				s->target_path, s->def->min_parts );
			return -1;
		}

		// will we have unused sources?
		if( s->def->max_parts > 0 && s->parts > s->def->max_parts )
		{
			warn( "Synthetic %s has %d sources but only %d will be used!",
				s->target_path, s->parts, s->def->max_parts );
		}

		if( s->enable )
		{
			// make a new synth and copy the contents
			// yes we are copying the pointers
			ns = (SYNTH *) mem_perm( sizeof( SYNTH ) );
			*ns = *s;

			// and link it
			ns->next = ctl->synth->list;
			ctl->synth->list = ns;
			++(ctl->synth->scount);

			debug( "Added synthetic %s (%s/%d).", ns->target_path,
				ns->def->names[0], ns->parts );
		}
		else
		{
			// tidy up
			if( s->target_path )
			{
				debug( "Dropping disabled synthetic %s (%s/%d).",
					s->target_path, s->def->names[0], s->parts );

				free( s->target_path );
				s->target_path = NULL;
			}

			for( i = 0; i < s->parts; ++i )
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

