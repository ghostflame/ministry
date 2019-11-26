/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* target/http.c - network targets http interface                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


// http interface


void __target_http_list( json_object *o, int enval )
{
	json_object *jl, *jt;
	char ebuf[512];
	TGTL *l;
	TGT *t;

	for( l = _tgt->lists; l; l = l->next )
	{
		// see if we have anything in this list that matches
		for( t = l->targets; t; t = t->next )
			if( t->enabled == enval )
				break;

		if( !t )
			continue;

		jl = json_object_new_array( );

		for( t = l->targets; t; t = t->next )
		{
			if( t->enabled != enval )
				continue;

			snprintf( ebuf, 512, "%s:%hu", t->host, t->port );

			jt = json_object_new_object( );

			json_insert( jt, "name",     string, t->name );
			json_insert( jt, "endpoint", string, ebuf );
			json_insert( jt, "type",     string, t->typestr );
			json_insert( jt, "bytes",    int,    t->bytes );

			json_object_array_add( jl, jt );
		}

		json_object_object_add( o, l->name, jl );
	}
}


int target_http_list( HTREQ *req )
{
	json_object *jo, *je, *jd;

	jo = json_object_new_object( );
	je = json_object_new_object( );
	jd = json_object_new_object( );

	__target_http_list( je, 1 );
	__target_http_list( jd, 0 );

	json_object_object_add( jo, "enabled",  je );
	json_object_object_add( jo, "disabled", jd );

	strbuf_json( req->text, jo, 1 );

	return 0;
}


int target_http_toggle( HTREQ *req )
{
	json_object *je, *jl, *jt;
	char *list, *trgt;
	TGTL *l;
	TGT *t;
	int e;

	if( !( je = json_object_object_get( req->post->jo, "enabled" ) )
	 || !( jl = json_object_object_get( req->post->jo, "list" ) )
	 || !( jt = json_object_object_get( req->post->jo, "target" ) ) )
	{
		create_json_result( req->text, 0, "Invalid json passed to %s.", req->path->path );
		return 0;
	}

	e = json_object_get_boolean( je );
	list = (char *) json_object_get_string( jl );
	trgt = (char *) json_object_get_string( jt );

	if( !( l = target_list_find( list ) )
	 || !( t = target_list_search( l, trgt, 0 ) ) )
	{
		create_json_result( req->text, 0, "Unknown target '%s/%s'", list, trgt );
		return 0;
	}

	if( t->enabled != e )
	{
		t->enabled = e;

		create_json_result( req->text, 1, "Target %s/%s %sabled.",
			l->name, t->name, ( e ) ? "en" : "dis" );

		notice( "Target %s/%s %sabled.",
			l->name, t->name, ( e ) ? "en" : "dis" );

		target_list_check_enabled( l );
	}
	else
	{
		create_json_result( req->text, 1, "Target %s/%s was already %sabled.",
			l->name, t->name, ( e ) ? "en" : "dis" );
	}

	return 0;
}


