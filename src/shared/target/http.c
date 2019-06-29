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


void __target_http_list( BUF *b, int enval )
{
	char ebuf[512];
	TGTL *l;
	TGT *t;
	int e;

	for( l = _tgt->lists; l; l = l->next )
	{
		// see if we have anything in this list that matches
		e = 0;
		for( t = l->targets; t; t = t->next )
			if( t->enabled == enval )
			{
				e = 1;
				break;
			}

		if( e )
		{
			json_flda( l->name );
			for( t = l->targets; t; t = t->next )
			{
				if( t->enabled == enval )
				{
					snprintf( ebuf, 512, "%s:%hu", t->host, t->port );

					json_starto( );

					json_flds( "name", t->name );
					json_flds( "endpoint", ebuf );
					json_flds( "type", t->typestr );
					json_fldI( "bytes", t->bytes );

					json_endo( );
				}
			}
			json_enda( );
		}
	}
}


int target_http_list( HTREQ *req )
{
	BUF *b = req->text;

	strbuf_resize( req->text, 32760 );

	json_starto( );

	json_fldo( "enabled" );
	__target_http_list( req->text, 1 );
	json_endo( );

	json_fldo( "disabled" );
	__target_http_list( req->text, 0 );
	json_endo( );

	json_finisho( );

	return 0;
}


int target_http_toggle( HTREQ *req )
{
	AVP *av = &(req->post->kv);
	TGTALT *ta;

	if( !req->post->objFree )
	{
		req->post->objFree = (TGTALT *) allocz( sizeof( TGTALT ) );
		strbuf_copy( req->text, "Target not found.\n", 0 );
	}

	ta = (TGTALT *) req->post->objFree;

	if( !strcasecmp( av->aptr, "enabled" ) )
	{
		ta->state = config_bool( av );
		ta->state_set = 1;
	}
	else if( !strcasecmp( av->aptr, "list" ) )
	{
		ta->list = target_list_find( av->vptr );
	}
	else if( !strcasecmp( av->aptr, "target" ) )
	{
		ta->tgt = target_list_search( ta->list, av->vptr, av->vlen );
	}
	else
		return 0;

	// do we have everything?
	// set the state then
	if( ta->list
	 && ta->tgt
	 && ta->state_set )
	{
		if( ta->tgt->enabled != ta->state )
		{
			ta->tgt->enabled = ta->state;

			strbuf_printf( req->text, "Target %s/%s %sabled.\n",
				ta->list->name, ta->tgt->name,
				( ta->state ) ? "en" : "dis" );

			notice( "Target %s/%s %sabled.",
				ta->list->name, ta->tgt->name,
				( ta->state ) ? "en" : "dis" );

			target_list_check_enabled( ta->list );
		}
		else
		{
			strbuf_printf( req->text, "Target %s/%s was already %sabled.\n",
				ta->list->name, ta->tgt->name,
				( ta->state ) ? "en" : "dis" );
		}
	}

	return 0;
}


