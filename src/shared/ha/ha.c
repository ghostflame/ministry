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

	info( "Calling set_master on %s.", p->name );

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


int ha_watcher_cb_setting( HAPT *p, WORDS *w )
{
	HA_CTL *ha = _proc->ha;
	int sett, ev;

	if( ( sett = ha_clst_line_sett_value( w->wd[1] ) ) < 0 )
	{
		warn( "Unrecognised cluster setting from %s: %s", p->name, w->wd[1] );
		return -1;
	}

	switch( sett )
	{
		case HA_CLST_LINE_SETT_ELECTOR:
			if( ( ev = ha_elector_value( w->wd[2] ) ) < 0 )
			{
				warn( "Unrecognised elector setting from %s: %s", p->name, w->wd[2] );
				return -1;
			}
			if( ev != ha->elector )
			{
				err( "Mismatched elector scheme!  We use %s, %s uses %s!",
					ha_elector_name( ha->elector ), p->name,
					ha_elector_name( ev ) );
				return -1;
			}
			else
				info( "Agree on elector scheme %s.", ha_elector_name( ev ) );
				
			break;

		default:
			break;
	}

	return 0;
}


int ha_watcher_cb_partner( HAPT *p, WORDS *w )
{
	int is_m, is_s;
	HAPT *q;

	if( w->wc != 4 )
	{
		warn( "Found invalid PARTNER line in cluster report from %s.", p->name );
		return -1;
	}

	if( !( q = ha_find_partner( w->wd[1], w->len[1] ) ) )
	{
		warn( "Cannot find partner '%s'", w->wd[1] );
		return -1;
	}

	if( p != q )
	{
		info( "Ignoring status of %s.", q->name );
		return -1;
	}

	is_m = atoi( w->wd[2] );
	is_s = atoi( w->wd[3] );

	if( is_s && ( p != q ) )
	{
		warn( "Found invalid PARTNER (self) line in cluster report from %s.", p->name );
		return -1;
	}

	// is this one master?
	if( is_m )
		ha_set_master( p );

	return 0;
}





void ha_watcher_cb( void *arg, IOBUF *b )
{
	HAPT *p = (HAPT *) arg;
	char *nl, *s;
	int l, kind;
	WORDS w;

	s = b->buf;

	while( b->len > 0 )
	{
		if( !( nl = memchr( s, '\n', b->len ) ) )
		{
			warn( "Found partial line in cluster report from %s: '%s'",
				p->name, s );
			break;
		}

		l = nl - s;
		*nl++ = '\0';

		if( strwords( &w, s, l, ' ' ) < 2 )
		{
			warn( "Found invalid line in cluster report from %s.", p->name );
			break;
		}

		b->len -= l + 1;
		s = nl;

		if( ( kind = ha_clst_line_kind_value( w.wd[0] ) ) < 0 )
		{
			warn( "Found invalid kind in cluster report from %s: %s", p->name, w.wd[0] );
			break;
		}

		switch( kind )
		{
			case HA_CLST_LINE_KIND_SETTING:

				if( ha_watcher_cb_setting( p, &w ) < 0 )
					return;

				break;

			case HA_CLST_LINE_KIND_STATUS:
				break;

			case HA_CLST_LINE_KIND_PARTNER:

				if( ha_watcher_cb_partner( p, &w ) < 0 )
					continue;

				break;

			default:
				break;
		}
	}
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
	p->ch->cb     = &ha_watcher_cb;
	p->ch->iobuf  = p->buf;

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
	HA_CTL *ha = _proc->ha;
	HAPT *p;

	// fits in the default 4k
	strbuf_aprintf( req->text, "STATUS UPTIME %ld\n", (int64_t) get_uptime_sec( ) );
	strbuf_aprintf( req->text, "SETTING ELECTOR %s\n", ha_elector_name( ha->elector ) );
	strbuf_aprintf( req->text, "SETTING PERIOD %ld\n", ha->period / 1000 );
	strbuf_aprintf( req->text, "SETTING MASTER %d\n", ha->is_master );

	for( p = ha->partners; p; p = p->next )
		strbuf_aprintf( req->text, "PARTNER %s %d %d\n", p->name, p->is_master,
				( p == ha->self ) ? 1 : 0 );

	return 0;
}


int ha_ctl_cluster( HTREQ *req )
{
	strbuf_copy( req->text, "Not implemented yet.\n", 0 );
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

	http_add_simple_get( DEFAULT_HA_CHECK_PATH, "Fetch cluster status", &ha_get_cluster );
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

