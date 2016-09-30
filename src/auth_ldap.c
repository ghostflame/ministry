/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* ldap.c - handles ldap connection and auth                               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"


// choose a server and lock it to make it busy
// sent it the credentials for verification
// record the result
int auth_ldap_verify_user( char *user, char *pass, int *result )
{
	LDAP_CTL *l = ctl->ldap;
	int offset, i, j;
	LDAP_SVR *s;

	if( !result )
		return -1;

	if( !user || !*user )
	{
		*result = AUTH_LDAP_NO_USER;
		return -1;
	}

	if( !pass || !*pass )
	{
		*result = AUTH_LDAP_NO_PASS;
		return -1;
	}

	// choose a server
	offset = rand_val( l->scount );

	for( i = 0; i < l->scount; i++ )
	{
		j = ( i + offset ) % l->scount;
		s = l->slist[j];

		if( !s->ready )
			continue;

		break;
	}

	// did we find one?
	if( i == l->scount )
	{
		*result = AUTH_LDAP_NOT_READY;
		return -1;
	}

	// now we can try looking up a user/pass combination

	return 0;
}




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

	debug( "Attempting to init ldap server %s", s->uri );

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

		debug( "Initialized ldap server %s", s->uri );
		s->tries = 0;
	}

	// are we bound?
	if( ldap_simple_bind_s( s->hdl, s->login, s->pass ) != LDAP_SUCCESS )
	{
		if( ++(s->tries) >= l->max_auth )
		{
			warn( "Failed to auth to ldap %s after %d tries, entering cooldown.",
				s->uri, l->max_auth );
			s->cooldown = l->cool_auth;
		}

		return;
	}

	debug( "Bound to ldap server %s", s->uri );

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
	if( s->hdl && s->ready )
	{
		debug( "Unbinding ldap server %s.", s->uri );
		ldap_unbind( s->hdl );
	}

	// clean up
	free( t );
	return NULL;
}



void auth_ldap_start( void )
{
	LDAP_CTL *l = ctl->ldap;
	struct timeval tv;
	LDAP_SVR *s;
	int j;

	if( !l->scount )
	{
		warn( "No LDAP servers configured, disabling LDAP." );
		l->enabled = 0;
	}

	if( !l->enabled )
		return;

	tv.tv_sec  = l->tmout_msec / 1000;
	tv.tv_usec = 1000 * ( l->tmout_msec % 1000 );

	ldap_set_option( NULL, LDAP_OPT_PROTOCOL_VERSION, &(l->version) );
	ldap_set_option( NULL, LDAP_OPT_NETWORK_TIMEOUT,  &tv );

	// make our flat list
	l->slist = (LDAP_SVR **) allocz( l->scount * sizeof( LDAP_SVR * ) );
	for( j = 0, s = l->servers; s; s = s->next )
		l->slist[j++] = s;

	// light them up
	for( s = l->servers; s; s = s->next )
		thread_throw( auth_ldap_server_control, s );
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



static LDAP_SVR __auth_ldap_cfg_tmp;
static int __auth_ldap_cfg_state = 0;

int auth_ldap_config_line( AVP *av )
{
	LDAP_SVR *ns, *s = &__auth_ldap_cfg_tmp;
	LDAP_CTL *l = ctl->ldap;
	char buf[1024];
	int j;

	// empty?
	if( !__auth_ldap_cfg_state )
		memset( s, 0, sizeof( LDAP_SVR ) );

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
	// server definitions
	else if( attIs( "host" ) )
	{
		if( s->host )
		{
			warn( "Hostname %s found against server with existing hostname %s",
				av->val, s->host );
			free( s->host );
		}

		s->host = str_copy( av->val, av->vlen );
		__auth_ldap_cfg_state = 1;
	}
	else if( attIs( "path" ) )
	{
		if( s->path )
		{
			warn( "Path found against server %s with existing path.",
				( s->host ) ? s->host :
				( s->uri )  ? s->uri  : "unknown" );
			free( s->path );
		}

		s->path = str_copy( av->val, av->vlen );
		__auth_ldap_cfg_state = 1;
	}
	else if( attIs( "port" ) )
	{
		s->port = (uint16_t) strtoul( av->val, NULL, 10 );
		__auth_ldap_cfg_state = 1;
	}
	else if( attIs( "login" ) || attIs( "user" ) )
	{
		if( s->login )
		{
			warn( "Login found against server %s with existing login.",
				( s->host ) ? s->host :
				( s->uri )  ? s->uri  : "unknown" );
			free( s->login );
		}

		s->login = str_copy( av->val, av->vlen );
		__auth_ldap_cfg_state = 1;
	}
	else if( attIs( "pass" ) || attIs( "password" ) )
	{
		if( s->pass )
		{
			warn( "Password found against server %s with existing password.",
				( s->host ) ? s->host : "unknown" );
			free( s->pass );
		}

		s->pass = str_copy( av->val, av->vlen );
		__auth_ldap_cfg_state = 1;
	}
	else if( attIs( "uri" ) )
	{
		if( s->uri )
		{
			warn( "URI found against server %s with existing URI.",
				( s->host ) ? s->host : s->uri );
			free( s->uri );
		}

		s->uri = str_copy( av->val, av->vlen );

		// is that a secure URI?
		if( !strncmp( s->uri, "ldaps:", 6 ) )
			s->secure = 1;

		__auth_ldap_cfg_state = 1;
	}
	else if( attIs( "secure" ) )
	{
		s->secure = config_bool( av );
		__auth_ldap_cfg_state = 1;
	}
	else if( attIs( "done" ) )
	{
		if( !s->uri && !s->host )
		{
			err( "No host or URI specified for LDAP server." );
			return -1;
		}

		if( !s->login || !s->pass )
		{
			warn( "No credentials specified for LDAP server %s.",
				( s->host ) ? s->host : s->uri );
		}

		// we have a port?
		if( !s->port )
			s->port = ( s->secure ) ? DEFAULT_SLDAP_PORT : DEFAULT_LDAP_PORT;

		// work out our URI if we don't have one
		if( !s->uri )
		{
			j = snprintf( buf, 1024, "ldap%s://%s:%hu/%s",
					( s->secure ) ? "s" : "",
					s->host, s->port,
					( s->path ) ? s->path : "" );

			s->uri = str_copy( buf, j );
		}

		// copy it
		ns = (LDAP_SVR *) allocz( sizeof( LDAP_SVR ) );
		memcpy( ns, s, sizeof( LDAP_SVR ) );

		// ready for another
		__auth_ldap_cfg_state = 0;

		// add that to the list
		ns->next   = l->servers;
		l->servers = ns;
		l->scount++;
	}
	else
		return -1;


	return 0;
}


