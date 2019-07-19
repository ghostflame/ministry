/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* http/handlers.c - functions to set/control url handlers                 *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"



HTPATH *http_find_callback( const char *url, int rlen, HTHDLS *hd )
{
	HTPATH *p;

	for( p = hd->list; p; p = p->next )
		if( p->plen == rlen && !memcmp( p->path, url, rlen ) )
			return p;

	return NULL;
}


int __http_add_handler( char *path, char *desc, void *arg, int method, http_callback *fp, IPLIST *srcs, int flags )
{
	HTHDLS *hd;
	HTPATH *p;
	int len;

	if( method == HTTP_METH_GET )
		hd = _http->get_h;
	else if( method == HTTP_METH_POST )
		hd = _http->post_h;
	else
	{
		err( "Only GET and POST are supported (%d - %s)", method, path );
		return -1;
	}

	if( !path || !*path || !fp )
	{
		err( "No path or handler function provided." );
		return -1;
	}

	if( *path != '/' )
	{
		err( "Provided path does not begin with /." );
		return -1;
	}

	if( !desc || !*desc )
		desc = "Unknown purpose";

	len = strlen( path );

	if( http_find_callback( path, len, hd ) )
	{
		err( "Duplicate path (%s) handler provided for '%s'.", hd->method, path );
		return -1;
	}

	p = (HTPATH *) allocz( sizeof( HTPATH ) );
	p->plen  = len;
	p->path  = str_dup( path, len );
	p->desc  = str_dup( desc, 0 );
	p->arg   = arg;
	p->flags = flags;
	p->list  = hd;
	p->srcs  = srcs;
	p->cb    = fp;

	p->next  = hd->list;
	hd->list = p;

	hd->count++;
	_http->hdlr_count++;

	// re-sort that list
	mem_sort_list( (void **) &(hd->list), hd->count, __http_cmp_handlers );
	debug( "Added %s%s handler: (%d/%hu) %s  ->  %s", hd->method,
	        ( p->flags & HTTP_FLAGS_CONTROL ) ? " (CONTROL)" : "",
	        hd->count, _http->hdlr_count, p->path, p->desc );

	return 0;
}


int http_add_handler( char *path, char *desc, void *arg, int method, http_callback *fp, IPLIST *srcs, int flags )
{
	return __http_add_handler( path, desc, arg, method, fp, srcs, flags );
}

int http_add_control( char *path, char *desc, void *arg, http_callback *fp, IPLIST *srcs, int flags )
{
	char urlbuf[1024];

	if( *path == '/' )
		path++;

	snprintf( urlbuf, 1024, "/control/%s", path );

	return __http_add_handler( urlbuf, desc, arg, HTTP_METH_POST, fp, srcs, flags|HTTP_FLAGS_CONTROL|HTTP_FLAGS_JSON );
}

// set an extra stats handler
int http_stats_handler( json_callback *fp )
{
	if( fp )
	{
		_http->stats_fp = fp;
		return 0;
	}

	return -1;
}





