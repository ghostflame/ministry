/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* synth/config.c - synthetic metrics functions and config                 *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

SYN_CTL *_syn = NULL;


SYN_CTL *synth_config_defaults( void )
{
	_syn = mem_perm( sizeof( SYN_CTL ) );
	return _syn;
}


static SYNTH __synth_cfg_tmp;
static int __synth_cfg_state = 0;


struct synth_fn_def synth_fn_defs[] =
{
	// the first name is the 'proper' name
	{ { "sum",    "add",     "plus" }, &synth_sum,    1, 0, SYNTH_PART_MAX },
	{ { "diff",   "minus",   NULL   }, &synth_diff,   2, 2, 0 },
	{ { "ratio",  "div",     NULL   }, &synth_div,    2, 2, 0 },
	{ { "mult",   "product", NULL   }, &synth_prod,   1, 0, SYNTH_PART_MAX },
	{ { "cap",    "limit",   NULL   }, &synth_cap,    2, 2, 0 },
	{ { "max",    "highest", NULL   }, &synth_max,    2, 0, SYNTH_PART_MAX },
	{ { "min",    "lowest",  NULL   }, &synth_min,    2, 0, SYNTH_PART_MAX },
	{ { "spread", "width",   NULL   }, &synth_spread, 2, 0, -2 },
	{ { "mean",   "average", NULL   }, &synth_mean,   2, 0, -1 },
	{ { "meanIf", "avgIf",   NULL   }, &synth_meanIf, 2, 0, -1 },
	{ { "count",  "nonzero", NULL   }, &synth_count,  1, 0, SYNTH_PART_MAX },
	{ { "active", "present", NULL   }, &synth_active, 1, 0, SYNTH_PART_MAX },
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
			ns->next = _syn->list;
			_syn->list = ns;
			++(_syn->scount);

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

