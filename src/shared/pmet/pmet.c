/**************************************************************************
* Copyright 2015 John Denholm                                             *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
*                                                                         *
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


void pmet_scrape_check( int64_t tval, void *arg )
{
	int64_t sd, ad, nt;

	nt = _proc->curr_time.tv_sec;
	sd = nt - _pmet->last_scrape;
	ad = nt - _pmet->last_alert;

	debug( "Scrape delta: %ld, alert delta: %ld, alert period: %ld", sd, ad, _pmet->alert_period );

	if( sd > _pmet->alert_period && ad > _pmet->alert_period )
	{
		warn( "Scrape expectations not met - not been scraped in %ld seconds.", sd );
		_pmet->last_alert = nt;
	}
}

void pmet_scrape_loop( THRD *t )
{
	// set last-scrape to now, so we don't alert at once
	_pmet->last_scrape = _proc->curr_time.tv_sec;
	// alert delta left to be big, so that we alert at once

	info( "Beginning scrape expectation check, threshold %ld sec.", _pmet->alert_period );

	loop_control( "pmet_check", pmet_scrape_check, NULL, _pmet->period, LOOP_TRIM|LOOP_SYNC, _pmet->period / 3 );
}


void pmet_report( BUF *into )
{
	strbuf_empty( into );

	// last scrape time, to the nearest second
	_pmet->last_scrape = _proc->curr_time.tv_sec;

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

	loop_control( "pmet_gen", pmet_pass, NULL, _pmet->period, LOOP_TRIM|LOOP_SYNC, _pmet->period / 4 );
}


int pmet_init( void )
{
	PMET_CTL *p = _pmet;
	int8_t i;

	if( !p->enabled )
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

	p->shared->upmet  = pmet_new( PMET_TYPE_COUNTER, "process_uptime_seconds", "How long the process has been up" );
	p->shared->uptime = pmet_create_gen( p->shared->upmet, p->shared->source, PMET_GEN_FN, NULL, &pmet_get_uptime, NULL );

	p->shared->memmet = pmet_new( PMET_TYPE_GAUGE, "process_memory_usage_bytes", "How many bytes used in memory" );
	p->shared->mem    = pmet_create_gen( p->shared->memmet, p->shared->source, PMET_GEN_FN, NULL, &pmet_get_memory, NULL );

	p->shared->cfgmet = pmet_new( PMET_TYPE_GAUGE, "ministry_config_changed", "Has the config on disk been changed" );
	p->shared->cfgChg = pmet_create_gen( p->shared->cfgmet, p->shared->source, PMET_GEN_IVAL, &(_proc->cfgChanged), NULL, NULL );

	p->shared->logmet = pmet_new( PMET_TYPE_COUNTER, "ministry_log_level_count", "Count of logs at each level" );
	for( i = 0; i < LOG_LEVEL_MAX; ++i )
	{
		p->shared->logs[i] = pmet_create_gen( p->shared->logmet, p->shared->source, PMET_GEN_IVAL,
		                                      &(_proc->log->counts[i]), NULL, NULL);
		pmet_label_apply_item( pmet_label_create( "level", (char *) log_get_level_name( i ) ), p->shared->logs[i] );
	}


	p->sources = mem_reverse_list( p->sources );

	_pmet->period *= 1000; // convert to usec

	thread_throw_named( &pmet_run, NULL, 0, "pmet_gen" );

	if( _pmet->alert_period > 0 )
	{
		thread_throw_named( &pmet_scrape_loop, NULL, 0, "pmet_scrape_chk" );
	}

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

	ps = (PMETS *) mem_perm( sizeof( PMETS ) );
	ps->name = str_perm( name, l );
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




// http interface


int pmet_source_control( HTREQ *req )
{
	// TODO - parse the json, find out what to turn on/off
	create_json_result( req->text, 0, "Not implemented yet." );
	return 0;
}


int pmet_source_list( HTREQ *req )
{
	json_object *jo, *jr, *je, *jd, *js;
	PMETS *s;

	jo = json_object_new_object( );
	jr = json_object_new_object( );
	je = json_object_new_array( );
	jd = json_object_new_array( );

	for( s = _pmet->sources; s; s = s->next )
	{
		js = json_object_new_object( );

		json_insert( js, "name",      string, s->name );
		json_insert( js, "lastCount", int,    s->last_ct );

		json_object_array_add( ( pmets_enabled( s ) ) ? je : jd, js );
	}

	json_object_object_add( jr, "enabled",  je );
	json_object_object_add( jr, "disabled", jd );
	json_object_object_add( jo, "sources",  jr );

	strbuf_json( req->text, jo, 1 );

	return 0;
}

