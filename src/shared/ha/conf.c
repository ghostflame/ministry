/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* ha/conf.c - high-availability configuration functions                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#include "local.h"





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


