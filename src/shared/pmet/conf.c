/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* pmet/conf.c - functions to provide prometheus metrics config            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


PMET_TYPE pmet_types[PMET_TYPE_MAX] =
{
	{
		.type = PMET_TYPE_UNTYPED,
		.name = "untyped",
		.rndr = &pmet_counter_render,
		.valu = &pmet_counter_value
	},
	{
		.type = PMET_TYPE_COUNTER,
		.name = "counter",
		.rndr = &pmet_counter_render,
		.valu = &pmet_counter_value
	},
	{
		.type = PMET_TYPE_GAUGE,
		.name = "gauge",
		.rndr = &pmet_gauge_render,
		.valu = &pmet_gauge_value
	},
	{
		.type = PMET_TYPE_SUMMARY,
		.name = "summary",
		.rndr = &pmet_summary_render,
		.valu = &pmet_summary_value
	},
	{
		.type = PMET_TYPE_HISTOGRAM,
		.name = "histogram",
		.rndr = &pmet_histogram_render,
		.valu = &pmet_histogram_value
	}
};


PMET_CTL *_pmet = NULL;



PMET_CTL *pmet_config_defaults( void )
{
	PMET_CTL *p = (PMET_CTL *) allocz( sizeof( PMET_CTL ) );
	int v;

	p->period = DEFAULT_PMET_PERIOD_MSEC;
	p->enabled = 0;

	v = 1;
	p->srclookup = string_store_create( 0, "nano", &v );
	p->metlookup = string_store_create( 0, "tiny", &v );

	// give ourselves half a meg
	p->page = strbuf( DEFAULT_PMET_BUF_SZ );

	if( ( v = regcomp( &(p->path_check), PMET_PATH_CHK_RGX, REG_EXTENDED|REG_ICASE|REG_NOSUB ) ) )
	{
		char *errbuf = (char *) allocz( 2048 );

		regerror( v, &(p->path_check), errbuf, 2048 );
		fatal( "Could not compile prometheus metrics path check regex: %s", errbuf );
		return NULL;
	}

	pthread_mutex_init( &(p->genlock), NULL );

	_pmet = p;

	// add in the basics
	p->shared = (PMET_SH *) allocz( sizeof( PMET_SH ) );
	p->shared->source = pmet_add_source( "shared" );

	return p;
}


int pmet_config_line( AVP *av )
{
	PMET_CTL *p = _pmet;
	int64_t v;
	SSTE *e;

	if( attIs( "enable" ) )
	{
		p->enabled = config_bool( av );
	}
	else if( attIs( "period" ) || attIs( "interval" ) )
	{
		if( parse_number( av->vptr, &v, NULL ) == NUM_INVALID )
		{
			err( "Invalid prometheus metrics generation period '%s'.", av->vptr );
			return -1;
		}

		p->period = v;
	}
	else if( attIs( "disable" ) )
	{
		if( !( e = string_store_add( p->srclookup, av->vptr, av->vlen ) ) )
		{
			err( "Could not handle disabling prometheus metrics type '%s'.", av->vptr );
			return -1;
		}
		info( "Prometheus metrics type %s disabled.", av->vptr );
		e->val = 0;
	}
	else
		return -1;

	return 0;
}


