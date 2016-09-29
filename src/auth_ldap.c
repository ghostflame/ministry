/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* ldap.c - handles ldap connection and auth                               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"



void auth_ldap_server_connect( int64_t tval, void *arg )
{
	LDAP_SVR *s = (LDAP_SVR *) arg;
	LDAP_CTL *l = ctl->ldap;

	// we good?
	if( s->ready )
		return;

	if( s->cooldown > 0 )
	{
		s->cooldown--;
		return;
	}

	// do we even have a handle?
	if( !s->hdl )
	{
		if( ldap_initialize( &(s->hdl), s->uri ) != LDAP_SUCCESS )
		{
			if( ++(s->tries) >= l->max_conn )
			{
				warn( "Failed to initialize to %s after %d tries, entering cooldown.",
					s->uri, l->max_conn );
				s->cooldown = l->cool_conn;
			}

			return;
		}
		else
			s->tries = 0;
	}

	/*
	// are we bound?
	if( ldap_bind_s( s->hdl, s->login, s->pass, LDAP_AUTH_SIMPLE ) != LDAP_SUCCESS )
	{
		if( ++(s->tries) >= l->max_auth )
		{
			warn( "Failed to auth to ldap %s:%hu after %d tries, entering cooldown.",
				s->host, s->port, l->max_auth );
			s->cooldown = l->cool_auth;
		}

		return;
	}
	*/

	// all good
	s->tries = 0;
	s->ready = 1;
	s->cooldown = 0;
}



void *auth_ldap_server_control( void *arg )
{
	LDAP_CTL *l;
	LDAP_SVR *s;
	THRD *t;
	int us;

	t = (THRD *) arg;
	s = (LDAP_SVR *) t->arg;
	l = ctl->ldap;

	// let's try and connect.  Never mind the result
	auth_ldap_server_connect( 0, s );

	// get our period in usec, desync'd a little
	us = ( l->loop_msec * 1000 ) - 105 + ( random( ) % 211 );

	// and loop
	loop_control( "ldap server", auth_ldap_server_connect, s, us, 0, 0 );

	// is it connected when we finish?
	//if( s->hdl && s->ready )
	//	ldap_unbind_s( s->hdl );

	// clean up
	free( t );
	return NULL;
}



int auth_ldap_start( void )
{
	LDAP_CTL *l = ctl->ldap;
	struct timeval tv;
	char uribuf[1024];
	LDAP_SVR *s;
	int j;

	if( l->enabled )
	{
		tv.tv_sec  = l->tmout_msec / 1000;
		tv.tv_usec = 1000 * ( l->tmout_msec % 1000 );

		ldap_set_option( NULL, LDAP_OPT_PROTOCOL_VERSION, &(l->version) );
		ldap_set_option( NULL, LDAP_OPT_NETWORK_TIMEOUT,  &tv );

		// light them up
		for( s = l->servers; s; s = s->next )
		{
			// pick a port number
			if( !s->port )
				s->port = ( l->secure ) ? DEFAULT_SLDAP_PORT : DEFAULT_LDAP_PORT;

			j = snprintf( uribuf, 1024, "ldap%s://%s:%hu/",
				( l->secure ) ? "s" : "",
				s->host, s->port );

			s->uri = str_dup( uribuf, j );

			thread_throw( auth_ldap_server_control, s );
		}
	}

	return 0;
}


// default log config
LDAP_CTL *auth_ldap_config_defaults( void )
{
	LDAP_CTL *l;

	l = (LDAP_CTL *) allocz( sizeof( LDAP_CTL ) );

	l->enabled          = 0;
	l->version          = DEFAULT_LDAP_VERSION;
	l->tmout_msec       = DEFAULT_LDAP_TMOUT_MSEC;
	l->loop_msec        = DEFAULT_LDAP_LOOP_MSEC;
	l->max_conn         = DEFAULT_LDAP_MAX_CONN;
	l->max_auth         = DEFAULT_LDAP_MAX_AUTH;
	l->cool_conn        = DEFAULT_LDAP_COOL_CONN;
	l->cool_auth        = DEFAULT_LDAP_COOL_AUTH;

	return l;
}


LDAP_SVR *__auth_ldap_config_svr = NULL;


int auth_ldap_config_line( AVP *av )
{
	LDAP_CTL *l = ctl->ldap;
	//LDAP_SVR *s;

	if( attIs( "enable" ) )
	{
		l->enabled = config_bool( av );
		info( "LDAP is %sabled.", ( l->enabled ) ? "en" : "dis" );
	}
	else if( attIs( "version" ) )
	{
		l->version = atoi( av->val );
		if( l->version != 2 && l->version != 3 )
		{
			err( "Unrecognised version of LDAP - %d", l->version );
			return -1;
		}
	}
	else if( attIs( "timeout" ) )
	{
		l->tmout_msec = atoi( av->val );
		if( l->tmout_msec <= 0 )
		{
			err( "Invalid LDAP network connect timeout - %dms", l->tmout_msec );
			return -1;
		}
	}
	else if( attIs( "loop" ) )
	{
		l->loop_msec = atoi( av->val );
		info( "LDAP loop timer set to %dms", l->loop_msec );
	}
	else
		return -1;


	return 0;
}


