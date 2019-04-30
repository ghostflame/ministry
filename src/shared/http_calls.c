/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* http_calls.c - built-in HTTP endpoints                                  *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "shared.h"


#define _jstr( s )				strbuf_aprintf( b, "%s\r\n", s )
#define _jfielda( p, f )		strbuf_aprintf( b, p "\"%s\": [\r\n", f )
#define _jfieldo( p, f )		strbuf_aprintf( b, p "\"%s\": {\r\n", f )
#define _jfields( p, f, v )		strbuf_aprintf( b, p "\"%s\": \"%s\",\r\n", f, v )
#define _jfieldd( p, f, v )		strbuf_aprintf( b, p "\"%s\": %f,\r\n", f, v )
#define _jfield4( p, f, v )		strbuf_aprintf( b, p "\"%s\": %d,\r\n", f, v )
#define _jfield8( p, f, v )		strbuf_aprintf( b, p "\"%s\": %ld,\r\n", f, v )
#define _jfieldu( p, f, v )		strbuf_aprintf( b, p "\"%s\": %u,\r\n", f, v )
#define _jfieldl( p, f, v )		strbuf_aprintf( b, p "\"%s\": %lu,\r\n", f, v )
#define _jprunecma( )			if( strbuf_lastchar( b ) == '\n' ) { strbuf_chopn( b, 3 ); }

#define _jmtype( n, mc )		snprintf( tmp, 16, "%s_calls", n ); \
								_jfieldl( "      ", tmp, mc.ctr ); \
								snprintf( tmp, 16, "%s_total", n ); \
								_jfieldl( "      ", tmp, mc.sum )


int http_calls_stats( HTREQ *req )
{
#ifdef MTYPE_TRACING
	char tmp[16];
#endif
	MTSTAT ms;
	BUF *b;
	int j;

	b = ( req->text = strbuf_resize( req->text, _proc->http->statsBufSize ) );

	_jstr( "{" );

	_jfields( "  ", "app", _proc->app_name );
	_jfieldd( "  ", "uptime", get_uptime( ) );
	_jfield8( "  ", "mem", mem_curr_kb( ) );

	_jfielda( "  ", "memtypes" );

	for( j = 0; j < _proc->mem->type_ct; j++ )
	{
		if( mem_type_stats( j, &ms ) != 0 )
			continue;

		_jstr( "    {" );
		_jfields( "      ", "type", ms.name );
		_jfieldu( "      ", "free", ms.ctrs.fcount );
		_jfieldu( "      ", "alloc", ms.ctrs.total );
		_jfieldl( "      ", "kb", ms.bytes >> 10 );
#ifdef MTYPE_TRACING
		_jmtype( "alloc",  ms.ctrs.all );
		_jmtype( "freed",  ms.ctrs.fre );
		_jmtype( "preall", ms.ctrs.pre );
		_jmtype( "refill", ms.ctrs.ref );
#endif
		_jprunecma( );
		_jstr( "    }," );
	}
	_jprunecma( );
	_jstr( "  ]" );
	_jstr( "}" );

	return 0;
}

#undef _jstr
#undef _jfielda
#undef _jfieldo
#undef _jfields
#undef _jfieldd
#undef _jfield4
#undef _jfield8
#undef _jfieldu
#undef _jfieldl
#undef _jprunecma



void __http_calls_usage_type( HTHDLS *hd, BUF *b )
{
	HTPATH *p;

	if( hd->count )
	{
		strbuf_aprintf( b, "[%s]\n", hd->method );

		for( p = hd->list; p; p = p->next )
			strbuf_aprintf( b, "%-16s %s\n", p->path, p->desc );
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


void http_calls_init( void )
{
	http_add_handler( "/", "Usage information", NULL, HTTP_METH_GET, &http_calls_usage, NULL, NULL );

	// you should disable this if you are writing your own
	if( _proc->http->stats )
		http_add_handler( "/stats", "Internal stats", NULL, HTTP_METH_GET, &http_calls_stats, NULL, NULL );
}


