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
	double dur;
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
		json_insert( jpth, "count", int, p->fq->pcount );

		json_object_array_add( res, jpth );
	}

	// this takes care of freeing things
	strbuf_json( req->text, res, 1 );

	// get the timing
	dur = ((double) get_time64( ) - q->q_when) / MILLIONF;
	// and report that value
	pmet_value( _qry->metrics->pm_tmg, dur );

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

	if( http_request_get_param( req, QUERY_PARAM_DEBUG, &str ) )
	{
		if( str && *str && atoi( str ) )
			flagf_add( q, QUERY_FLAGS_DEBUG );
	}

	// let's go check concurrency
	if( query_add_current( ) != 0 )
	{
		strbuf_copy( req->text, "Too many concurrent queries -- try again later.", 0 );
		req->code = MHD_HTTP_TOO_MANY_REQUESTS;
		return 1;
	}

	query_search( q );

	if( q->pcount == 0 )
	{
		//info( "Found no paths." );
		query_write( req, q );
		query_rem_current( );
		return 0;
	}

	if( http_request_get_param( req, QUERY_PARAM_TO, &str ) && str && *str )
	{
		// expect it in msec but who knows
		if( time_usec( str, &(q->end) ) != 0 )
		{
			query_rem_current( );
			req->code = MHD_HTTP_BAD_REQUEST;
			return 1;
		}
	}
	else
		q->end = _proc->curr_usec;

	// q->end is now safe to use

	// span overrides to
	if( http_request_get_param( req, QUERY_PARAM_SPAN, &str ) && str && *str )
	{
		if( time_span_usec( str, &usec ) )
		{
			strbuf_copy( req->text, "Invalid time span.", 0 );
			query_rem_current( );
			req->code = MHD_HTTP_BAD_REQUEST;
			return 1;
		}

		q->start = q->end - usec;
	}
	else
	{
		if( http_request_get_param( req, QUERY_PARAM_FROM, &str ) && str && *str )
		{
			// expect it in msec but who knows
			if( time_usec( str, &(q->start) ) != 0 )
			{
				query_rem_current( );
				req->code = MHD_HTTP_BAD_REQUEST;
				return 1;
			}
		}
		else
			q->start = q->end - _qry->default_timespan;
	}

	// q->start and q->end now safe

	if( http_request_get_param( req, QUERY_PARAM_METRIC, &str ) )
	{
		q->metric = rkv_choose_metric( str );
		if( q->metric < 0 )
		{
			// don't echo the param back out to the user - that's a
			// bad idea
			strbuf_copy( req->text, "Metric not recognised.", 0 );
			query_rem_current( );
			req->code = MHD_HTTP_BAD_REQUEST;
			return 1;
		}
	}

	if( flagf_has( q, QUERY_FLAGS_DEBUG ) )
	{
		debug( "Query created: %lld %lld for %s", q->start, q->end, q->search );
	}

	for( p = q->paths; p; p = p->next )
		query_path_get_data( p, q );

	query_write( req, q );
	query_rem_current( );

	return 0;
}

