/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* ha.c - high-availability functions                                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#include "shared.h"

//
//
//	Utilities
//
//

const char *ha_elector_name_strings[HA_ELECT_MAX] =
{
	"random", "first", "manual"
};

const char *ha_election_state_strings[HA_ELECTION_MAX] =
{
	"config", "choosing", "chosen", "mismatch"
};

const char *ha_clst_line_kind_strings[HA_CLST_LINE_KIND_MAX] =
{
	"STATUS", "SETTING", "PARTNER"
};

const char *ha_clst_line_setting_strings[HA_CLST_LINE_SETT_MAX] =
{
	"ELECTOR", "PERIOD", "UPDATE"
};

struct ha_string_set
{
	const char		**	arr;
	int					max;
};

struct ha_string_set ha_string_sets[4] =
{
	{	ha_elector_name_strings,		HA_ELECT_MAX			},
	{	ha_election_state_strings,		HA_ELECTION_MAX			},
	{	ha_clst_line_kind_strings,		HA_CLST_LINE_KIND_MAX	},
	{	ha_clst_line_setting_strings,	HA_CLST_LINE_SETT_MAX	}
};


const char *__ha_string_name( int which, int val )
{
	struct ha_string_set *hss = ha_string_sets + which;

	if( val < 0 || val >= hss->max )
		return NULL;

	return hss->arr[val];
}

int __ha_string_val( int which, char *str )
{
	struct ha_string_set *hss = ha_string_sets + which;
	int i;

	for( i = 0; i < hss->max; i++ )
		if( !strcasecmp( str, hss->arr[i] ) )
			return i;

	return -1;
}

#define ha_elector_name( _e )			__ha_string_name( 0, _e )
#define ha_elector_value( _n )			__ha_string_val( 0, _n )

#define ha_election_state_name( _s )	__ha_string_name( 1, _s )
#define ha_election_state_value( _n )	__ha_string_val( 1, _n )

#define ha_clst_line_kind_name( _k )	__ha_string_name( 2, _k )
#define ha_clst_line_kind_value( _n )	__ha_string_val( 2, _n )

#define ha_clst_line_sett_name( _s )	__ha_string_name( 3, _s )
#define ha_clst_line_sett_value( _n )	__ha_string_val( 3, _n )




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



//
//
//	Config
//
//

HAPT *ha_find_partner( char *name, int len )
{
	HA_CTL *ha = _proc->ha;
	HAPT *p;

	if( !len )
		len = strlen( name );

	for( p = ha->partners; p; p = p->next )
		if( len == p->nlen && !memcmp( p->name, name, len ) )
			return p;

	return NULL;
}


// uses libcurl to parse out the spec
HAPT *ha_add_partner( char *spec, int dupe_fail )
{
	HAPT *p, *q;
	CURLU *ch;
	char *str;
	int len;

	// set us up
	if( !( ch = curl_url( ) ) )
	{
		err( "Could not create curl handle." );
		return NULL;
	}

	if( curl_url_set( ch, CURLUPART_URL, spec, 0 ) )
	{
		err( "Could not set url '%s'", spec );
		goto FailURL;
	}

	p = (HAPT *) allocz( sizeof( HAPT ) );

	// pull out host and port, for easier reporting
	if( curl_url_get( ch, CURLUPART_HOST, &str, 0 ) )
	{
		err( "Could not get hostname from spec '%s'", spec );
		goto FailURL;
	}
	p->host = str_copy( str, 0 );
	curl_free( str );

	if( curl_url_get( ch, CURLUPART_PORT, &str, 0 ) )
	{
		err( "Could not get port from spec '%s'", spec );
		goto FailURL;
	}
	p->port = (uint16_t) strtoul( str, NULL, 10 );
	curl_free( str );


	// figure out if it's TLS - we might need client certs
	// one day (when that works...)
	if( curl_url_get( ch, CURLUPART_SCHEME, &str, 0 ) )
	{
		err( "Could not get scheme from spec '%s'", spec );
		goto FailURL;
	}
	if( !strcmp( str, "https" ) )
		p->use_tls = 1;
	curl_free( str );


	// check for duplicates
	for( q = _proc->ha->partners; q; q = q->next )
	{
		if( q->port == p->port
		 && !strcmp( q->host, p->host ) )
		{
			if( dupe_fail )
			{
				err( "Partner %s:%hu already exists.",
					q->host, q->port );
				goto FailURL;
			}

			// always fail on TLS mismatch
			if( q->use_tls != p->use_tls )
			{
				err( "Partner %s:%hu found, but with TLS mismatch.",
					q->host, q->port );
				goto FailURL;
			}

			curl_url_cleanup( ch );
			return p;
		}
	}

	// set the check path, and read the url back out
	if( curl_url_set( ch, CURLUPART_PATH, DEFAULT_HA_CHECK_PATH, 0 ) )
	{
		err( "Could not set status check path on spec '%s'", spec );
		goto FailURL;
	}
	if( curl_url_get( ch, CURLUPART_URL, &str, 0 ) )
	{
		err( "Could not read back check path url on spec '%s'", spec );
		goto FailURL;
	}
	p->check_url = str_copy( str, 0 );
	curl_free( str );

	if( curl_url_set( ch, CURLUPART_PATH, DEFAULT_HA_CTL_PATH, 0 ) )
	{
		err( "Could not set status control path on spec '%s'", spec );
		goto FailURL;
	}
	if( curl_url_get( ch, CURLUPART_URL, &str, 0 ) )
	{
		err( "Could not read back control path url on spec '%s'", spec );
		goto FailURL;
	}
	p->ctl_url = str_copy( str, 0 );
	curl_free( str );

	curl_url_cleanup( ch );

	len = strlen( p->host ) + 6;
	p->name = perm_str( len );
	p->nlen = snprintf( p->name, len + 1, "%s:%hu", p->host, p->port );

	p->next = _proc->ha->partners;
	_proc->ha->partners = p;
	_proc->ha->pcount++;

	return p;


FailURL:
	curl_url_cleanup( ch );
	return NULL;
}



HA_CTL *ha_config_defaults( void )
{
	HA_CTL *ha = (HA_CTL *) allocz( sizeof( HA_CTL ) );

	ha->enabled = 0;
	// we default to being master, this enables solo behaviour
	ha->is_master = 1;
	ha->priority  = 1000;
	ha->elector = HA_ELECT_FIRST;
	ha->period = DEFAULT_HA_CHECK_MSEC;
	ha->update = DEFAULT_HA_UPDATE_MSEC;

	// ideally we would throw the self-partner now, but we need
	// config, not least http config, including finding out if
	// http is active.

	return ha;
}


int ha_config_line( AVP *av )
{
	HA_CTL *ha = _proc->ha;
	int64_t v, i;

	if( attIs( "enable" ) )
	{
		ha->enabled = config_bool( av );
		//info( "High-Availability %sabled.", ( ha->enabled ) ? "en" : "dis" );
	}
	else if( attIs( "hostname" ) )
	{
		ha->hostname = str_copy( av->vptr, av->vlen );
	}
	else if( attIs( "partner" ) || attIs( "member" ) )
	{
		if( !ha_add_partner( av->vptr, 1 ) )
			return -1;
	}
	else if( attIs( "master" ) )
	{
		ha->is_master = config_bool( av );
		//info( "High-Availability - would start as %s.",
		//	( ha->is_master ) ? "primary" : "secondary" );
	}
	else if( attIs( "checkPeriod" ) )
	{
		if( parse_number( av->vptr, &v, NULL ) == NUM_INVALID )
		{
			err( "Invalid check period msec '%s'", av->vptr );
			return -1;
		}
		ha->period = v;
	}
	else if( attIs( "updatePeriod" ) )
	{
		if( parse_number( av->vptr, &v, NULL ) == NUM_INVALID )
		{
			err( "Invalid update period msec '%s'", av->vptr );
			return -1;
		}
		ha->update = v;
	}
	else if( attIs( "elector" ) )
	{
		if( ( i = ha_elector_value( av->vptr ) ) < 0 )
		{
			char errbuf[1024];
			int l;

			l = snprintf( errbuf, 1024, "Unrecognised elector type, choose from:" );
			for( i = 0; i < HA_ELECT_MAX; i++ )
				l += snprintf( errbuf + l, 1024 - l, " %s", ha_elector_name_strings[i] );

			err( errbuf );
			return -1;
		}

		ha->elector = i;
	}
	else
		return -1;

	return 0;
}


