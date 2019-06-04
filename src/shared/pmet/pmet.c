/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* pmet.c - functions to provide prometheus metrics endpoint               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


/*
 * A note on metrics generation
 *
 * Most of our apps here keep stats for their own purposes.
 * They check their own memory, they report their own numbers.
 *
 * So I have avoided creating a prometheus metrics client, with metrics
 * pre-defined, and all of the bucketing and histograms etc that a full
 * client would do.
 *
 * The philosophy here is just to lift numbers of our pre-existing
 * places, and generate our page each time.
 *
 * Furthermore, by having multiple generator functions, I can break up
 * generation as much as I want, either having pmet_gen_shared pull in
 * all the shared code metrics, or have individual parts of the code
 * register their own.
 *
 * Certainly the apps will register their own functions, and the normal
 * ministry philosophy is to break up the generation there into pieces,
 * with each part of the code handling its own metrics.
 *
 * I have initially tied the generation interval to the stats reporting
 * interval, for sanity.  I highly recommend configuring your prometheus
 * to fetch on that interval - 10s.
 *
 * To that end, ministry apps report the timestamp with their metrics.
 * Details of the exposition format can be found in the latest prometheus
 * documentation here:
 * https://github.com/prometheus/docs/blob/master/content/docs/instrumentin
g/exposition_formats.md
 *
 * Finally, I am in favour of generating prometheus metrics on a clock,
 * *not* on demand.  If you run multiple prometheus servers for high
 * availability, it is a constant frustation when they see different
 * data and so when graphing is load-balanced across them, graphs jump
 * around from refresh to refresh.  So ministry apps will not add to
 * that situation - the apps make a metrics page, and prometheuses
 * come and get it or not.
 *
 */


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





void pmet_pass( int64_t tval, void *arg )
{
	PMET_CTL *p = _pmet;
	PMETS *s;
	PMETM *m;

	p->timestamp = tval / MILLION; // we just need msec

	//debug( "Generating prometheus metrics." );

	pmet_genlock( );

	// run across each source, clearing the counter
	for( s = p->sources; s; s = s->next )
		s->last_ct = 0;

	strbuf_empty( p->page );

	// run across each metric, generating data
	// for them.
	for( m = p->metrics; m; m = m->next )
		pmet_metric_gen( p->timestamp, m );

	// then render
	for( m = p->metrics; m; m = m->next )
		pmet_metric_render( p->timestamp, p->page, m );

	pmet_genunlock( );

	//debug( "Generated sources, outsz %u.", p->page->len );
}



void pmet_report( BUF *into )
{
	strbuf_empty( into );

	if( !_pmet->enabled )
		return;

	pmet_genlock( );

	strbuf_resize( into, _pmet->page->len );
	strbuf_copy( into, _pmet->page->buf, _pmet->page->len );

	pmet_genunlock( );
}



void pmet_run( THRD *t )
{
	notice( "Beginning generation of prometheus metrics." );

	_pmet->period *= 1000; // convert to usec

	loop_control( "pmet_gen", pmet_pass, NULL, _pmet->period, LOOP_TRIM|LOOP_SYNC, _pmet->period / 2 );
}


int pmet_init( void )
{
	if( !_pmet->enabled )
	{
		info( "Prometheus metrics are disabled." );
		return 0;
	}

	// no point without the http server as well
	if( !_proc->http->enabled )
	{
		warn( "Prometheus metrics are enabled, but the HTTP server is not; disabling prometheus metrics." );
		_pmet->enabled = 0;
		return 0;
	}

	_pmet->sources = mem_reverse_list( _pmet->sources );

	thread_throw_named( &pmet_run, NULL, 0, "pmet_gen" );

	// and add our paths
	http_add_control( "pmet", "Control prometheus metrics generation", NULL, &pmet_source_control, NULL, 0 );
	http_add_json_get( "/show/pmet", "Show prometheus metrics generation", &pmet_source_list );

	return 0;
}



PMETS *pmet_add_source( char *name )
{
	PMETS *ps;
	SSTE *sse;
	int l;

	if( !name || !*name )
	{
		err( "No function pointer or name given to pmet_add_source." );
		return NULL;
	}

	l = strlen( name );

	// we might already have one, it depends when things get registered,
	// because it might be disabled in config
	if( !( sse = string_store_add( _pmet->srclookup, name, l ) ) )
	{
		err( "Could not add name '%s' to the prometheus metrics string store.",
			name );
		return NULL;
	}

	ps = (PMETS *) allocz( sizeof( PMETS ) );
	ps->name = str_dup( name, l );
	ps->nlen = l;
	ps->sse  = sse;

	// link ourself up
	ps->sse->ptr = ps;

	// and turn ourself on
	ps->sse->val = 1;

	ps->next = _pmet->sources;
	_pmet->sources = ps;

	return ps;
}



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
	p->shared = pmet_add_source( "shared" );

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



// http interface


int pmet_source_control( HTREQ *req )
{
	return 0;
}


int pmet_source_list( HTREQ *req )
{
	BUF *b = req->text;
	PMETS *s;

	json_starto( );
	json_flda( "sources" );
	for( s = _pmet->sources; s; s = s->next )
	{
		json_fldo( s->name );
		json_fldi( "enabled", pmets_enabled( s ) );
		json_fldi( "lastCount", s->last_ct );
		json_endo( );
	}
	json_enda( );
	json_finisho( );

	return 0;
}

