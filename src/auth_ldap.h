/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* auth_ldap.h - defines LDAP structures and functions                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_AUTH_LDAP_H
#define MINISTRY_AUTH_LDAP_H

// let's figure out the new stuff in a bit
#define LDAP_DEPRECATED				1

#define DEFAULT_LDAP_PORT			389
#define DEFAULT_SLDAP_PORT			636

#define DEFAULT_LDAP_VERSION		LDAP_VERSION3
#define DEFAULT_LDAP_MAX_CONN		5
#define DEFAULT_LDAP_MAX_AUTH		3
#define DEFAULT_LDAP_COOL_CONN		180
#define DEFAULT_LDAP_COOL_AUTH		600
#define DEFAULT_LDAP_TMOUT_MSEC		10000
#define DEFAULT_LDAP_LOOP_MSEC		1000

#define MAX_LDAP_HOSTS				7


struct auth_ldap_server
{
	LDAP_SVR				*	next;

	char					*	host;
	char					*	login;
	char					*	pass;
	char					*	uri;

	uint16_t					port;

	LDAP					*	hdl;

	int							ready;
	int							busy;

	int							tries;
	int							cooldown;

	pthread_mutex_t				lock;
};


struct auth_ldap_control
{
	LDAP_SVR				*	servers;
	LDAP_SVR				**	slist;
	int							scount;

	int							enabled;
	int							version;
	int							secure;

	int							loop_msec;
	int							tmout_msec;

	int							max_conn;
	int							max_auth;

	int							cool_conn;
	int							cool_auth;
};


loop_call_fn auth_ldap_server_connect;
throw_fn auth_ldap_server_control;


int auth_ldap_start( void );


LDAP_CTL *auth_ldap_config_defaults( void );
int auth_ldap_config_line( AVP *av );

#endif

