/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* http_calls.c - built-in HTTP endpoints                                  *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "shared.h"



int http_calls_metrics( HTREQ *req )
{



	return 0;
}




#define _jmtype( n, mc )		snprintf( tmp, 16, "%s_calls", n ); \
								json_fldU( tmp, mc.ctr ); \
								snprintf( tmp, 16, "%s_total", n ); \
								json_fldU( tmp, mc.sum )


int http_calls_stats( HTREQ *req )
{
#ifdef MTYPE_TRACING
	char tmp[16];
#endif
	MTSTAT ms;
	BUF *b;
	int j;

	b = ( req->text = strbuf_resize( req->text, 8000 ) );

	json_starto( );

	json_flds( "app", _proc->app_name );
	json_fldf( "uptime", get_uptime( ) );
	json_fldI( "mem", mem_curr_kb( ) );

	json_fldo( "memTypes" );

	for( j = 0; j < _proc->mem->type_ct; j++ )
	{
		if( mem_type_stats( j, &ms ) != 0 )
			continue;

		json_fldo( ms.name );
		json_fldu( "free", ms.ctrs.fcount );
		json_fldu( "alloc", ms.ctrs.total );
		json_fldU( "kb", ms.bytes >> 10 );
#ifdef MTYPE_TRACING
		_jmtype( "alloc",  ms.ctrs.all );
		_jmtype( "freed",  ms.ctrs.fre );
		_jmtype( "preall", ms.ctrs.pre );
		_jmtype( "refill", ms.ctrs.ref );
#endif
		json_endo( );
	}
	json_endo( );

	if( _proc->http->stats_fp )
		(*(_proc->http->stats_fp))( req );

	json_finisho( );

	return 0;
}



void __http_calls_count_one( BUF *b, HTPATH *pt )
{
	json_starto( );
	json_flds( "path", pt->path );
	json_flds( "description", pt->desc );
	json_flds( "iplist", ( pt->iplist ) ? pt->iplist : "(none)" );
	json_fldI( "hits", pt->hits );
	json_endo( );
}


int http_calls_count( HTREQ *req )
{
	HTPATH *pt;
	BUF *b;

	b = ( req->text = strbuf_resize( req->text, 8000 ) );

	json_starto( );

	json_flda( "GET" );

	for( pt = _proc->http->get_h->list; pt; pt = pt->next )
		__http_calls_count_one( b, pt );

	json_enda( );
	json_flda( "POST" );

	for( pt = _proc->http->post_h->list; pt; pt = pt->next )
		__http_calls_count_one( b, pt );

	json_enda( );
	json_finisho( );

	return 0;
}




void __http_calls_usage_type( HTHDLS *hd, BUF *b )
{
	HTPATH *p;

	if( hd->count )
	{
		strbuf_aprintf( b, "[%s]\n", hd->method );

		for( p = hd->list; p; p = p->next )
			strbuf_aprintf( b, "%-24s %s\n", p->path, p->desc );
	}
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
			strbuf_aprintf( req->text, "%-24s %s\n", p->path, p->desc );

	return 0;
}






int http_calls_ctl_init( HTREQ *req )
{
	req->pproc = MHD_create_post_processor( req->conn, DEFAULT_POST_BUF_SZ,
	                 &http_calls_ctl_iterator, req );

	return 0;
}

int http_calls_ctl_iterator( void *cls, enum MHD_ValueKind kind, const char *key, const char *filename,
        const char *content_type, const char *transfer_encoding, const char *data, uint64_t off, size_t size )
{
	HTREQ *req = (HTREQ *) cls;

	req->post->kv.aptr = (char *) key;
	req->post->kv.alen = strlen( key );

	req->post->kv.vptr = (char *) data;
	req->post->kv.vlen = (int) size;

	if( (req->path->cb)( req ) < 0 )
		return MHD_NO;

	return MHD_YES;
}


int http_calls_ctl_done( HTREQ *req )
{
	MHD_destroy_post_processor( req->pproc );
	req->pproc = NULL;

	return 0;
}



// setup
void http_calls_init( void )
{
	http_add_simple_get( "/", "Usage information", &http_calls_usage );

	http_add_json_get( "/stats", "Internal stats", &http_calls_stats );

	http_add_json_get( "/counts", "HTTP request counts", &http_calls_count );

	// prometheus support
	http_add_simple_get( "/metrics", "HTTP prometheus metrics endpoint", &http_calls_metrics );

	// add target control
	http_add_json_get( "/targets", "List metric targets", &target_http_list );
	http_add_simple_get( "/control", "List control paths", &http_calls_ctl_list );
	http_add_control( "target", "Enable/disable target", NULL, &target_http_toggle, NULL, 0 );

	// ha
	http_add_simple_get( DEFAULT_HA_CHECK_PATH, "Fetch cluster status", &ha_get_cluster );
	http_add_control( "cluster", "Control cluster status", NULL, &ha_ctl_cluster, NULL, HTTP_FLAGS_NO_REPORT );
}


