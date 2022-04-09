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
* query/config.c - query config functions                                 *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


QRY_CTL *_qry = NULL;


void query_close( void )
{
	_qry->use_lock = 0;
	usleep( 200000 );
	pthread_mutex_destroy( &(_qry->qlock) );
}


void query_init( void )
{
	QRMET *m;

	//http_add_handler( char *path, char *desc, void *arg, int method, http_callback *fp, IPLIST *srcs, int flags )

	http_add_handler( "/query", "Query the timeseries store", NULL, HTTP_METH_GET,
		&query_get_callback, NULL, HTTP_FLAGS_JSON );

	debug( "Query callback registered; max concurrency %d.", _qry->max_curr );

	// set up our metrics
	m = _qry->metrics;

	// these autogenerate
	m->pm_qry = pmet_create_gen( m->queries, m->source, PMET_GEN_IVAL, &(_qry->count), NULL, NULL );
	m->pm_cur = pmet_create_gen( m->current, m->source, PMET_GEN_IVAL, &(_qry->curr), NULL, NULL );
	// we'll feed this one values directly
	m->pm_tmg = pmet_create( m->timings, m->source );

	pthread_mutex_init( &(_qry->qlock), NULL );
	_qry->use_lock = 1;
}


QRY_CTL *query_config_defaults( void )
{
	_qry = (QRY_CTL *) allocz( sizeof( QRY_CTL ) );

	_qry->default_timespan = DEFAULT_QUERY_TIMESPAN * MILLION;
	_qry->max_paths = DEFAULT_QUERY_MAX_PATHS;
	_qry->max_curr = DEFAULT_QUERY_MAX_CURR;

	_qry->metrics			= (QRMET *) allocz( sizeof( QRMET ) );
	_qry->metrics->source	= pmet_add_source( "queries" );
	_qry->metrics->queries	= pmet_new( PMET_TYPE_COUNTER, "archivist_queries_total",
	                                    "Ministry archivist total number of queries since start" );
	_qry->metrics->current	= pmet_new( PMET_TYPE_GAUGE, "archivist_queries_current",
	                                    "Ministry archivist current number of concurrent queries" );
	// this one needs data posting to it
	_qry->metrics->timings	= pmet_new( PMET_TYPE_SUMMARY, "archivist_queries_times",
	                                    "Ministry archivist summary of query times" );


	return _qry;
}


int query_config_line( AVP *av )
{
	int64_t v;

	if( attIs( "defaultSpan" ) )
	{
		if( time_span_usec( av->vptr, &v ) != 0 )
		{
			err( "Invalid default query span: %s", av->vptr );
			return -1;
		}

		_qry->default_timespan = v;
	}
	else if( attIs( "maxPaths" ) )
	{
		if( parse_number( av->vptr, &v, NULL ) == NUM_INVALID )
		{
			err( "Invalid max query paths: %s", av->vptr );
			return -1;
		}

		_qry->max_paths = v;
	}
	else if( attIs( "queryLimit" ) )
	{
		if( parse_number( av->vptr, &v, NULL ) == NUM_INVALID )
		{
			err( "Invalid concurrent query limit: %s", av->vptr );
			return -1;
		}

		_qry->max_curr = v;
	}
	else
		return -1;

	return 0;
}


