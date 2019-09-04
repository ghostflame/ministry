/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* auth/conf.c - auth configurations functions                             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


AUTH_CTL *_auth = NULL;

AUTH_CTL *auth_config_defaults( void )
{
	AUTH_CTL *a = (AUTH_CTL *) allocz( sizeof( AUTH_CTL) );

	_auth = a;
	return a;
}


int auth_config_line( AVP *av )
{



	return 0;
}


