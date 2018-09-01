/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* fetch.c - handles fetching metrics                                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "ministry.h"

void fetch_single( int64_t tval, void *arg )
{
	FETCH *f = (FETCH *) arg;

	info( "Fetching for target %s", f->name );
}



void *fetch_loop( void *arg )
{
	FETCH *f;
	THRD *t;

	t = (THRD *) arg;
	f = (FETCH *) t->arg;
	// we don't use this again
	free( t );

	// clip the offset to the size of the period
	if( f->offset >= f->period )
	{
		warn( "Fetch %s offset larger than period - clipping it.", f->name );
		f->offset = f->offset % f->period;
	}

	// set our timing params, msec -> usec
	f->period *= 1000;
	f->offset *= 1000;

	// and run the loop
	loop_control( "fetch", &fetch_single, f, f->period, 0, f->offset );
	return NULL;
}



int fetch_init( void )
{
	int i = 0;
	FETCH *f;

	// fix the order, for what it matters
	ctl->fetch->targets = mem_reverse_list( ctl->fetch->targets );

	for( i = 0, f = ctl->fetch->targets; f; i++, f = f->next )
		thread_throw( fetch_loop, f, i );

	return ctl->fetch->fcount;
}




FTCH_CTL *fetch_config_defaults( void )
{
	FTCH_CTL *fc = (FTCH_CTL *) allocz( sizeof( FTCH_CTL ) );

	return fc;
}


static FETCH __fetch_config_tmp;
static int __fetch_config_state = 0;

int fetch_config_line( AVP *av )
{
	FETCH *n, *f = &__fetch_config_tmp;
	int64_t v;
	DTYPE *d;
	int i;

	if( !__fetch_config_state )
	{
		if( f->name )
			free( f->name );
		memset( f, 0, sizeof( FETCH ) );
		f->name = str_dup( "as-yet-unnamed", 0 );
	}

	if( attIs( "name" ) )
	{
		free( f->name );
		f->name = str_dup( av->val, av->vlen );
		__fetch_config_state = 1;
	}
	else if( attIs( "url" ) )
	{
		if( f->url )
		{
			warn( "Fetch block %s already had url: '%s'", f->name, f->url );
			free( f->url );
		}

		f->url = str_dup( av->val, av->vlen );
		__fetch_config_state = 1;
	}
	else if( attIs( "prometheus" ) )
	{
		f->metrics = config_bool( av );
	}
	else if( attIs( "type" ) )
	{
		f->dtype = NULL;

		for( i = 0, d = (DTYPE *) data_type_defns; i < DATA_TYPE_MAX; i++, d++ )
			if( !strcasecmp( d->name, av->val ) )
			{
				f->dtype = d;
				break;
			}

		if( !f->dtype )
		{
			err( "Type '%s' not recognised.", av->val );
			return -1;
		}
		if( f->metrics )
			warn( "Data type set to '%s' but this is a prometheus source.", f->dtype->name );
	}
	else if( attIs( "period" ) )
	{
		if( parse_number( av->val, &v, NULL ) == NUM_INVALID )
		{
			err( "Invalid fetch period in block %s: %s", f->name, av->val );
			return -1;
		}

		f->period = v;
	}
	else if( attIs( "offset" ) )
	{
		if( parse_number( av->val, &v, NULL ) == NUM_INVALID )
		{
			err( "Invalid fetch offset in block %s: %s", f->name, av->val );
			return -1;
		}

		f->offset = v;
	}
	else if( attIs( "done" ) )
	{
		if( !f->url )
		{
			err( "Fetch block %s has no url!", f->name );
			return -1;
		}
		if( !f->metrics && !f->dtype )
		{
			err( "Fetch block %s has no type!", f->name );
			return -1;
		}

		// copy it
		n = (FETCH *) allocz( sizeof( FETCH ) );
		memcpy( n, f, sizeof( FETCH ) );

		// zero the static struct
		memset( f, 0, sizeof( FETCH ) );
		__fetch_config_state = 0;

		// link it up
		n->next = ctl->fetch->targets;
		ctl->fetch->targets = n;
		ctl->fetch->fcount++;
	}
	else
		return -1;

	return 0;
}
