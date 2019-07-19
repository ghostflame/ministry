/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
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
	HA_CTL *ha = _proc->ha;
	HAPT *q;

	//info( "Calling set_master on %s.", p->name );

	// already master?
	if( p->is_master )
		return;

	lock_ha( );

	// every one except p and self, step down partner
	for( q = ha->partners; q; q = q->next )
		if( q != p && q->is_master )
		{
			if( q == ha->self )
			{
				notice( "Stepping down from being primary." );
				ha->is_master = 0;
			}
			else
			{
				notice( "Stepping down partner %s from being primary.", q->name );
			}
			q->is_master = 0;
		}

	// step up p, which might be self
	if( p == ha->self )
	{
		notice( "Stepping up to be primary." );
		ha->is_master = 1;
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

	for( i = 0; i < pc; i++ )
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

	p->ch->iobuf->len = 0;

	if( curlw_fetch( p->ch ) != 0 )
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
	HA_CTL *ha = _proc->ha;

	// guess the curl timeout as half the period
	// we may adjust this later as we learn about
	// the target server
	p->tmout_orig = ha->period / 2;
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

	loop_control( "ha_watcher", ha_watcher_pass, td->arg, 1000 * ha->period, LOOP_TRIM, 0 );
}

void ha_controller( THRD *td )
{
	HA_CTL *ha = _proc->ha;

	//info( "HA controller started." );

	// offset by a chunk of the check period
	loop_control( "ha_controller", ha_controller_pass, NULL, 1000 * ha->update, LOOP_TRIM, 300 * ha->period );

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
	HA_CTL *ha = _proc->ha;
	HAPT *p;

	jo = json_object_new_object( );
	json_object_object_add( jo, "uptime",  json_object_new_int( (int64_t) get_uptime_sec( ) ) );
	json_object_object_add( jo, "elector", json_object_new_string( ha_elector_name( ha->elector ) ) );
	json_object_object_add( jo, "period",  json_object_new_int( ha->period / 1000 ) );
	json_object_object_add( jo, "master",  json_object_new_boolean( ha->is_master ) );

	ja = json_object_new_array( );

	for( p = ha->partners; p; p = p->next )
	{
		jp = json_object_new_object( );
		json_object_object_add( jp, "name",   json_object_new_string( p->name ) );
		json_object_object_add( jp, "master", json_object_new_boolean( p->is_master ) );
		json_object_object_add( jp, "self",   json_object_new_boolean( ( p == ha->self ) ) );
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
	HA_CTL *ha = _proc->ha;
	char buf[1024];

	if( !ha->enabled )
		return 0;

	// put ourself into the group
	if( !ha->hostname )
		ha->hostname = _proc->hostname;

	snprintf( buf, 1024, "http%s://%s:%hu/",
		( http_tls_enabled( ) ) ? "s" : "",
		ha->hostname, _proc->http->server_port );

	if( !( ha->self = ha_add_partner( buf, 0 ) ) )
	{
		err( "Cannot add ourself as part of a cluster." );
		return -1;
	}
	ha->self->is_master = ha->is_master;

	// sort the partners and make a flat list
	// this ensures all copies of ministry have identical lists
	// if they were given the same config
	mem_sort_list( (void **) &(ha->partners), ha->pcount, ha_cmp_partners );

	pthread_mutex_init( &(ha->lock), NULL );

	http_add_json_get( DEFAULT_HA_CHECK_PATH, "Fetch cluster status", &ha_get_cluster );
	http_add_control( "cluster", "Control cluster status", NULL, &ha_ctl_cluster, NULL, HTTP_FLAGS_NO_REPORT );

	return 0;
}


int ha_start( void )
{
	HA_CTL *ha = _proc->ha;
	HAPT *p;
	int i;

	if( !ha->enabled )
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
	for( i = 0, p = ha->partners; p; p = p->next, i++ )
		if( p != ha->self )
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
	if( !_proc->ha->enabled )
		return;

	pthread_mutex_destroy( &(_proc->ha->lock) );
}


