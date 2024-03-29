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
* http/calls.c - built-in HTTP endpoints                                  *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"



int http_calls_health( HTREQ *req )
{
	json_object *jo;
	int hret = 0;

	jo = json_object_new_object( );

	if( _proc->http->health_fp )
		hret = (*(_proc->http->health_fp))( jo );

	strbuf_json( req->text, jo, 1 );

	// did we just return failure?
	if( hret < 0 )
		req->code = 500;
	// let the health check specify a code
	else if( hret > 1 )
		req->code = hret;

	return 0;
}




int http_calls_metrics( HTREQ *req )
{
	// this does all the hard work
	pmet_report( req->text );

	return 0;
}




#define _jmtype( n, mc )		snprintf( tmp, 16, "%s_calls", n ); \
								json_insert( jt, tmp, int64, mc.ctr ); \
								snprintf( tmp, 16, "%s_total", n ); \
								json_insert( jt, tmp, int64, mc.sum )


int http_calls_stats( HTREQ *req )
{
	json_object *jo, *jm, *jt;
#ifdef MTYPE_TRACING
	char tmp[16];
#endif
	char upt[24];
	MTSTAT ms;
	int j;

	jo = json_object_new_object( );
	jm = json_object_new_object( );

	get_uptime_msec( upt, 24 );

	json_insert( jo, "app",    string, _proc->app_name );
	json_insert( jo, "uptime", string, upt );
	json_insert( jo, "mem",    int64,  mem_curr_kb( ) );

	for( j = 0; j < _proc->mem->type_ct; ++j )
	{
		if( mem_type_stats( j, &ms ) != 0 )
			continue;

		jt = json_object_new_object( );

		json_insert( jt, "free",  int,   ms.ctrs.fcount );
		json_insert( jt, "alloc", int,   ms.ctrs.total );
		json_insert( jt, "kb",    int64, ( ms.bytes >> 10 ) );

#ifdef MTYPE_TRACING
		_jmtype( "alloc",  ms.ctrs.all );
		_jmtype( "freed",  ms.ctrs.fre );
		_jmtype( "preall", ms.ctrs.pre );
		_jmtype( "refill", ms.ctrs.ref );
#endif

		json_object_object_add( jm, ms.name, jt );
	}

	json_object_object_add( jo, "memTypes", jm );

	if( _proc->http->stats_fp )
		(*(_proc->http->stats_fp))( jo );

	strbuf_json( req->text, jo, 1 );

	return 0;
}


int http_calls_version( HTREQ *req )
{
	json_object *jo;
	char upt[24];

	get_uptime_msec( upt, 24 );

	jo = json_object_new_object( );
	json_insert( jo, "app",     string, _proc->app_name );
	json_insert( jo, "uptime",  string, upt );
	json_insert( jo, "version", string, _proc->version );

	strbuf_json( req->text, jo, 1 );
	return 0;
}




void __http_calls_count_one( json_object *a, HTPATH *pt )
{
	json_object *o;

	o = json_object_new_object( );

	json_insert( o, "path",        string, pt->path );
	json_insert( o, "description", string, pt->desc );
	json_insert( o, "iplist",      string, ( ( pt->iplist ) ? pt->iplist : "(none)" ) );
	json_insert( o, "hits",        int64,  pt->hits );

	json_object_array_add( a, o );
}


int http_calls_count( HTREQ *req )
{
	json_object *jo, *jg, *jp;
	HTPATH *pt;

	jo = json_object_new_object( );
	jg = json_object_new_array( );
	jp = json_object_new_array( );

	for( pt = _proc->http->get_h->list; pt; pt = pt->next )
		__http_calls_count_one( jg, pt );

	for( pt = _proc->http->post_h->list; pt; pt = pt->next )
		__http_calls_count_one( jp, pt );

	json_object_object_add( jo, "GET",  jg );
	json_object_object_add( jo, "POST", jp );

	strbuf_json( req->text, jo, 1 );

	return 0;
}




void __http_calls_usage_type( HTHDLS *hd, BUF *b )
{
	char *jflag;
	HTPATH *p;

	if( hd->count )
	{
		strbuf_aprintf( b, "[%s]\n", hd->method );

		for( p = hd->list; p; p = p->next )
		{
			jflag = ( p->flags & HTTP_FLAGS_JSON ) ? "json" : "    ";
			strbuf_aprintf( b, "%-24s  %s  %s\n", p->path, jflag, p->desc );
		}
	}
}

int http_calls_time( HTREQ *req )
{
	double d;

	d = timedbl( get_time64( ) );
	strbuf_printf( req->text, "%f\n", d );

	return 0;
}


int http_calls_usage( HTREQ *req )
{
	req->text = strbuf_resize( req->text, 8100 );
	strbuf_empty( req->text );

	__http_calls_usage_type( _proc->http->get_h,  req->text );
	__http_calls_usage_type( _proc->http->post_h, req->text );

	return 0;
}



// controls
int http_calls_ctl_list( HTREQ *req )
{
	HTHDLS *h = _proc->http->post_h;
	HTPATH *p;

	req->text = strbuf_resize( req->text, 8100 );
	strbuf_empty( req->text );

	for( p = h->list; p; p = p->next )
		if( p->flags & HTTP_FLAGS_CONTROL )
			strbuf_aprintf( req->text, "%-24s  json  %s\n", p->path, p->desc );

	return 0;
}



int http_calls_json_init( HTREQ *req )
{
	FILE *tfh;

	if( !( tfh = tmpfile( ) ) )
	{
		err( "Could not open tmpfile for upload." );
		return -1;
	}

	//info( "Opened a filehandle for a new request." );
	req->post->obj = tfh;
	return 0;
}

int http_calls_json_data( HTREQ *req )
{
	FILE *tfh;

	if( !( tfh = (FILE *) req->post->obj ) )
	{
		err( "No file handle on POST data." );
		return -1;
	}

	//info( "Wrote %ld bytes to request filehandle.", req->post->bytes );
	return fwrite( req->post->data, req->post->bytes, 1, tfh ) - 1;
}

int http_calls_json_done( HTREQ *req )
{
	int ret = 0;
	FILE *tfh;

	if( !( tfh = (FILE *) req->post->obj ) )
	{
		err( "No file handle on POST data." );
		return -1;
	}

	// did we finish badly?
	if( req->err )
	{
		fclose( tfh );
		req->post->obj = NULL;
		return -1;
	}

	//info( "Parsing request upload file." );

	// let's try to parse what we've got
	if( ( req->post->jo = parse_json_file( tfh, NULL ) ) )
	{
		// and hit the callback
		ret = (req->path->cb)( req );
	}
	else
	{
		req->code = MHD_HTTP_UNPROCESSABLE_CONTENT;
		ret = -1;
	}

	fclose( tfh );
	req->post->obj = NULL;

	return ret;
}



// setup
void http_calls_init( void )
{
	http_add_simple_get( "/", "Usage information", &http_calls_usage );
	http_add_simple_get( "/time", "Get the current time", &http_calls_time );

	http_add_json_get( "/stats", "Internal stats", &http_calls_stats );
	http_add_json_get( "/counts", "HTTP request counts", &http_calls_count );
	http_add_json_get( "/health", "Health check", &http_calls_health );

	if( !runf_has( RUN_NO_TARGET ) )
	{
		http_add_json_get( "/targets", "List metric targets", &target_http_list );
	}

	http_add_json_get( "/version", "Display version information", &http_calls_version );

	// prometheus support
	http_add_simple_get( "/metrics", "HTTP prometheus metrics endpoint", &http_calls_metrics );

	if( _http->ctl_enabled )
		http_add_simple_get( "/control", "List control paths", &http_calls_ctl_list );

	// add target control
	if( !runf_has( RUN_NO_TARGET ) )
	{
		http_add_control( "target", "Enable/disable target", NULL, &target_http_toggle, NULL, 0 );
	}
}


