/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* target.c - functions to write to different backend targets              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry_test.h"


struct targets_defaults targets_data[METRIC_TYPE_MAX] =
{
	{	METRIC_TYPE_ADDER,	DEFAULT_ADDER_PORT,		"adder"		},
	{	METRIC_TYPE_STATS,	DEFAULT_STATS_PORT,		"stats"		},
	{	METRIC_TYPE_GAUGE,	DEFAULT_GAUGE_PORT,		"gauge"		},
	{	METRIC_TYPE_HISTO,	DEFAULT_HISTO_PORT,		"histo"		},
	{	METRIC_TYPE_COMPAT,	DEFAULT_COMPAT_PORT,	"compat"	}
};


int targets_start_one( TGT **tp )
{
	TGT *t, *orig;

	// null?  do nothing
	if( !( orig = *tp ) )
	{
		notice( "Ignoring null target specifier." );
		return 0;
	}

	// make a new target, a clone of the original
	t = (TGT *) allocz( sizeof( TGT ) );
	memcpy( t, orig, sizeof( TGT ) );

	// run an io loop
	target_run_one( t, 0 );

	// and store that in the p2p
	*tp = t;

	return 0;
}


void targets_resolve( void )
{
	struct targets_defaults *td;
	TGTL *l;
	TGT *t;
	int i;

	td = &(targets_data[0]);

	for( i = 0; i < METRIC_TYPE_MAX; ++i, ++td )
		if( ( l = target_list_find( td->name ) ) )
		{
			if( l->targets )
			{
				for( t = l->targets; t; t = t->next )
				{
					debug( "Found %s target %s (%s:%hu) [%d]", td->name,
						t->name, t->host, t->port, flagf_val( t, TGT_FLAG_ENABLED ) );

					if( !t->port )
						t->port = td->port;
				}

				ctl->tgt->targets[i] = l->targets;
			}
			else
				warn( "Found no targets in the list for type %s.", td->name );
		}
		else
			warn( "Found no targets for type %s.", td->name );
}


int targets_set_type( TGT *t, char *name, int len )
{
	struct targets_defaults *td;
	int i;

	td = &(targets_data[0]);

	for( i = 0; i < METRIC_TYPE_MAX; ++i, ++td )
		if( !strcasecmp( name, td->name ) )
		{
			t->type = td->type;
			if( !t->port )
				t->port = td->port;
			ctl->tgt->targets[t->type] = t;
			return 0;
		}

	err( "Unrecognised target type: %s", name );
	return -1;
}


TGTS_CTL *targets_config_defaults( void )
{
	TGTS_CTL *t = (TGTS_CTL *) allocz( sizeof( TGTS_CTL ) );
	return t;
}


