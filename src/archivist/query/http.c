/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* query/http.c - query http callbacks                                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

void query_path_get_data( QP *p, QRY *q )
{
	LEAF *l;
	TEL *t;

	t = p->tel;
	l = t->leaf;

	if( !l->fh )
	{
		rkv_tree_lock( t );

		if( !l->fh )
			l->fh = rkv_create_handle( t->path, t->plen );

		rkv_tree_unlock( t );

		// TODO - handle persistent failures
		if( !l->fh )
		{
			warn( "Could not get a file handle for metric %s", t->path );
			return;
		}
	}

	p->fq = mem_new_rkqry( );
	p->fq->from   = q->start;
	p->fq->to     = q->end;
	p->fq->metric = q->metric;

	rkv_read( l->fh, p->fq );
}



void query_write( HTREQ *req, QRY *q )
{
	JSON *res, *jpth, *jpts, *jva, *jt, *jv;
	//int64_t ts;
	PNT *pt;
	QP *p;
	int j;

	res = json_object_new_array( );

	for( p = q->paths; p; p = p->next )
	{
		jpth = json_object_new_object( );
		jpts = json_object_new_array( );

		for( j = 0; j < p->fq->data->count; ++j )
		{
			pt = p->fq->data->points + j;

			// TODO - add null points
			if( pt->ts )
			{
				jva = json_object_new_array( );
				jt = json_object_new_int64( pt->ts );
				jv = json_object_new_double( pt->val );
				json_object_array_add( jva, jt );
				json_object_array_add( jva, jv );
				json_object_array_add( jpts, jva );
			}
		}

		json_insert( jpth, "target", string, p->tel->path );
		json_object_object_add( jpth, "datapoints", jpts );

		json_object_array_add( res, jpth );
	}

	// this takes care of freeing things
	strbuf_json( req->text, res, 1 );

	query_free( q );
}



int query_get_callback( HTREQ *req )
{
	int64_t usec;
	char *str;
	QRY *q;
	QP *p;

	if( !http_request_get_param( req, QUERY_PARAM_PATH, &str ) || !str || !*str )
	{
		req->code = MHD_HTTP_BAD_REQUEST;
		return 1;
	}

	if( !( q = query_create( str ) ) )
	{
		req->code = MHD_HTTP_BAD_REQUEST;
		return 1;
	}

	query_search( q );

	if( q->pcount == 0 )
	{
		//info( "Found no paths." );
		query_write( req, q );
		return 0;
	}

	q->end = _proc->curr_usec;

	if( http_request_get_param( req, QUERY_PARAM_TO, &str ) && str && *str )
	{
		// expect it in msec
		q->end = 1000 * strtoll( str, NULL, 10 );
	}

	if( http_request_get_param( req, QUERY_PARAM_SPAN, &str ) && str && *str )
	{
		if( time_span_usec( str, &usec ) )
		{
			strbuf_copy( req->text, "Invalid time span.", 0 );
			req->code = MHD_HTTP_BAD_REQUEST;
			return 1;
		}

		q->start = q->end - usec;
	}
	else
	{
		if( http_request_get_param( req, QUERY_PARAM_FROM, &str ) && str && *str )
		{
			// expect it in msec
			q->start = 1000 * strtoll( str, NULL, 10 );
		}
		else
			q->start = q->end - _qry->default_timespan;
	}

	if( http_request_get_param( req, QUERY_PARAM_METRIC, &str ) )
	{
		q->metric = rkv_choose_metric( str );
		if( q->metric < 0 )
		{
			// don't echo the param back out to the user - that's a
			// bad idea
			strbuf_copy( req->text, "Metric not recognised.", 0 );
			req->code = MHD_HTTP_BAD_REQUEST;
			return 1;
		}
	}


	for( p = q->paths; p; p = p->next )
		query_path_get_data( p, q );

	query_write( req, q );
	return 0;
}


