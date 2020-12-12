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
* ha/ha.c - high-availability server functions                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#include "local.h"




//
//
//	Runtime control
//
//

void ha_set_master( HAPT *p )
{
	HAPT *q;

	//info( "Calling set_master on %s.", p->name );

	// already master?
	if( p->is_master )
		return;

	lock_ha( );

	// every one except p and self, step down partner
	for( q = _ha->partners; q; q = q->next )
		if( q != p && q->is_master )
		{
			if( q == _ha->self )
			{
				notice( "Stepping down from being primary." );
				_ha->is_master = 0;
			}
			else
			{
				notice( "Stepping down partner %s from being primary.", q->name );
			}
			q->is_master = 0;
		}

	// step up p, which might be self
	if( p == _ha->self )
	{
		notice( "Stepping up to be primary." );
		_ha->is_master = 1;
	}
	else
	{
		notice( "Stepping up partner %s to being primary." );
	}
	p->is_master = 1;

	unlock_ha( );
}


int ha_watcher_cb_compare_elector( HAPT *p, const char *el )
{
	int ev;

	if( ( ev = ha_elector_value( (char *) el ) ) < 0 )
	{
		warn( "Unrecognised elector setting from %s: %s", p->name, el );
		return -1;
	}

	if( ev != _ha->elector )
	{
		err( "Mismatched elector scheme!  We use %s, %s uses %s!",
			ha_elector_name( _ha->elector ), p->name,
			ha_elector_name( ev ) );
		return -1;
	}
	//else
	//	info( "Agree on elector scheme %s.", ha_elector_name( ev ) );

	return 0;
}




void ha_watcher_jcb( void *arg, json_object *jo )
{
	json_object *o, *pn, *ps, *pm, *jp, *ja;
	HAPT *p, *pt = (HAPT *) arg;
	const char *str;
	size_t i, pc;

	if( !( o = json_object_object_get( jo, "elector" ) ) )
	{
		warn( "No elector found from partner %s.", pt->name );
		goto INVALID_JSON;
	}

	if( ha_watcher_cb_compare_elector( pt, json_object_get_string( o ) ) )
		return;

	if( !( ja = json_object_object_get( jo, "partners" ) ) )
	{
		warn( "No partner listing from partner %s.", pt->name );
		goto INVALID_JSON;
	}

	pc = json_object_array_length( ja );

	for( i = 0; i < pc; ++i )
	{
		jp = json_object_array_get_idx( ja, i );

		// examine the details
		if( !( pn = json_object_object_get( jp, "name" ) )
		 || !( ps = json_object_object_get( jp, "self" ) )
		 || !( pm = json_object_object_get( jp, "master" ) ) )
		{
			warn( "Invalid partner listing %d from partner %s.", i, pt->name );
			goto INVALID_JSON;
		}

		str = json_object_get_string( pn );
		if( !( p = ha_find_partner( str, json_object_get_string_len( pn ) ) ) )
		{
			warn( "Cannot find partner '%s' referenced from partner %s.", str, pt->name );
			goto INVALID_JSON;
		}

		// ignore it's view of other partners (for now)
		if( p != pt )
			continue;

		if( !json_object_get_boolean( ps ) )
		{
			warn( "Partner %s doesn't seem to recognise itself as itself.", pt->name );
			goto INVALID_JSON;
		}

		// is it master?
		if( json_object_get_boolean( pm ) )
		{
			//info( "Partner %s thinks it is master.", pt->name );
			ha_set_master( pt );
		}
	}

	return;

INVALID_JSON:
	warn( "Invalid json seen from partner %s.", pt->name );
}



void ha_watcher_pass( int64_t tval, void *arg )
{
	HAPT *p = (HAPT *) arg;

	strbuf_empty( p->ch->iobuf->bf );

	if( curlw_fetch( p->ch, NULL, 0 ) != 0 )
		warn( "Failed to get response from HA partner %s:%hu", p->host, p->port );
}

void ha_controller_pass( int64_t tval, void *arg )
{
	// implement the state machine.

	return;
}


void ha_watcher( THRD *td )
{
	HAPT *p = (HAPT *) td->arg;

	// guess the curl timeout as half the period
	// we may adjust this later as we learn about
	// the target server
	p->tmout_orig = _ha->period / 2;
	p->tmout      = p->tmout_orig;
	p->buf        = mem_new_iobuf( IO_BUF_SZ );

	p->ch         = (CURLWH *) allocz( sizeof( CURLWH ) );
	p->ch->url    = p->check_url;
	p->ch->arg    = p;	// set ourself as the argument
	p->ch->jcb    = &ha_watcher_jcb;
	p->ch->iobuf  = p->buf;

	// and say we want json parsing
	setCurlF( p->ch, PARSE_JSON );
	setCurlF( p->ch, VERBOSE );


	//info( "HA watcher starting for %s.", p->name );

	loop_control( "ha_watcher", ha_watcher_pass, td->arg, 1000 * _ha->period, LOOP_TRIM, 0 );
}

void ha_controller( THRD *td )
{
	//info( "HA controller started." );

	// offset by a chunk of the check period
	loop_control( "ha_controller", ha_controller_pass, NULL, 1000 * _ha->update, LOOP_TRIM, 300 * _ha->period );

	//info( "HA controller exiting." );
}




//
//
//	HTTP callbacks
//
//


int ha_get_cluster( HTREQ *req )
{
	json_object *jo, *ja, *jp;
	HAPT *p;

	jo = json_object_new_object( );
	json_insert( jo, "uptime",  int,     (int64_t) get_uptime_sec( ) );
	json_insert( jo, "elector", string,  ha_elector_name( _ha->elector ) );
	json_insert( jo, "period",  int,     _ha->period / 1000 );
	json_insert( jo, "master",  boolean, _ha->is_master );

	ja = json_object_new_array( );

	for( p = _ha->partners; p; p = p->next )
	{
		jp = json_object_new_object( );
		json_insert( jp, "name",   string,  p->name );
		json_insert( jp, "master", boolean, p->is_master );
		json_insert( jp, "self",   boolean, ( p == _ha->self ) );
		json_object_array_add( ja, jp );
	}

	json_object_object_add( jo, "partners", ja );

	strbuf_json( req->text, jo, 1 );

	return 0;
}


int ha_ctl_cluster( HTREQ *req )
{
	create_json_result( req->text, 0, "Not implemented yet." );
	return 0;
}




//
//
//	Init
//
//




int ha_cmp_partners( const void *p1, const void *p2 )
{
	HAPT *m1 = *((HAPT **) p1);
	HAPT *m2 = *((HAPT **) p2);
	int rv;

	if( ( rv = strcmp( m1->host, m2->host ) ) )
		return rv;

	return ( m1->port > m2->port ) ? 1 : ( m1->port == m2->port ) ? 0 : -1;
}


int ha_init( void )
{
	char buf[1024];

	if( !_ha->enabled )
		return 0;

	// put ourself into the group
	if( !_ha->hostname )
		_ha->hostname = _proc->hostname;

	snprintf( buf, 1024, "http%s://%s:%hu/",
		( http_tls_enabled( ) ) ? "s" : "",
		_ha->hostname, _proc->http->server_port );

	if( !( _ha->self = ha_add_partner( buf, 0 ) ) )
	{
		err( "Cannot add ourself as part of a cluster." );
		return -1;
	}
	_ha->self->is_master = _ha->is_master;

	// sort the partners and make a flat list
	// this ensures all copies of ministry have identical lists
	// if they were given the same config
	mem_sort_list( (void **) &(_ha->partners), _ha->pcount, ha_cmp_partners );

	pthread_mutex_init( &(_ha->lock), NULL );

	http_add_json_get( DEFAULT_HA_CHECK_PATH, "Fetch cluster status", &ha_get_cluster );
	http_add_control( "cluster", "Control cluster status", NULL, &ha_ctl_cluster, NULL, HTTP_FLAGS_NO_REPORT );

	return 0;
}


int ha_start( void )
{
	HAPT *p;
	int i;

	if( !_ha->enabled )
	{
		//info( "High-availablity is disabled." );
		return 0;
	}

	if( !_proc->http->enabled )
	{
		err( "Cannot have HA enabled with the HTTP server disabled." );
		return -1;
	}

	// run a watcher thread for each one
	for( i = 0, p = _ha->partners; p; p = p->next, ++i )
		if( p != _ha->self )
		{
			thread_throw_named_f( ha_watcher, p, i, "ha_watcher_%d", i );
			//notice( "Found HA partner on %s", p->name );
		}

	// and our controller
	thread_throw_named( ha_controller, NULL, 0, "ha_controller" );

	return 0;
}

void ha_shutdown( void )
{
	if( !_ha->enabled )
		return;

	pthread_mutex_destroy( &(_ha->lock) );
}


