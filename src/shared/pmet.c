/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* pmet.c - functions to provide prometheus metrics endpoint               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"


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



PMET_CTL *_pmet = NULL;



int pmet_gen_shared( BUF *b, void *arg )
{
	return 0;
}





void pmet_pass( int64_t tval, void *arg )
{
	PMET_CTL *p = _pmet;
	PMSRC *s;
	int c;

	p->outsz = 0;
	p->timestamp = tval;

	// run across each source, capturing
	// how many metrics it output
	for( s = p->sources; s; s = s->next )
	{
		strbuf_empty( s->buf );

		// is this one enabled?
		if( !s->sse->val )
			continue;

		if( ( c = (*(s->fp))( s->buf, s->sse->ptr ) ) < 0 )
			warn( "Failed to generate prometheus metrics for %s.", s->name );
		else
		{
			p->outsz += s->buf->len;
			s->last_ct = c;
		}
	}
}



void pmet_run( THRD *t )
{
	_pmet->period = 1000; // convert to usec
	loop_control( "pmet_gen", pmet_pass, NULL, _pmet->period, LOOP_TRIM|LOOP_SYNC, _pmet->period / 2 );
}


int pmet_init( void )
{
	if( !_pmet->enabled )
		return 0;

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



PMSRC *pmet_add_source( pmet_fn *fp, char *name, void *arg, int sz )
{
	PMSRC *ps;
	SSTE *sse;
	int l;

	if( !fp || !name || !*name )
	{
		err( "No function pointer or name given to pmet_add_source." );
		return NULL;
	}

	l = strlen( name );

	// we might already have one, it depends when things get registered,
	// because it might be disabled in config
	if( !( sse = string_store_add( _pmet->lookup, name, l ) ) )
	{
		err( "Could not add name '%s' to the prometheus metrics string store.",
			name );
		return NULL;
	}

	if( !sz )
		sz = DEFAULT_PMET_BUF_SZ;

	sse->ptr = arg;

	ps = (PMSRC *) allocz( sizeof( PMSRC ) );
	ps->name = str_dup( name, l );
	ps->nlen = l;
	ps->buf  = strbuf( sz );
	ps->sse  = sse;
	ps->fp   = fp;

	ps->next = _pmet->sources;
	_pmet->sources = ps;
	_pmet->scount++;

	return ps;
}



PMET_CTL *pmet_config_defaults( void )
{
	PMET_CTL *p = (PMET_CTL *) allocz( sizeof( PMET_CTL ) );
	int v = 1;

	p->period = DEFAULT_PMET_PERIOD_MSEC;
	p->enabled = 0;

	p->lookup = string_store_create( 0, "tiny", &v );

	// add in the basics
	p->shared = pmet_add_source( pmet_gen_shared, "shared", NULL, 0 );

	_pmet = p;

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
		if( !( e = string_store_add( p->lookup, av->vptr, av->vlen ) ) )
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
	PMSRC *s;

	json_starto( );
	json_flda( "types" );
	for( s = _pmet->sources; s; s = s->next )
	{
		json_fldo( s->name );
		json_fldi( "enabled", s->sse->val );
		json_fldi( "lastCount", s->last_ct );
		json_endo( );
	}
	json_enda( );
	json_finisho( );

	return 0;
}

