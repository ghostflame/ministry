/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* strings/conf.c - config for string handling                             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

STR_CTL *_str = NULL;


void string_deinit( void )
{
	pthread_mutex_destroy( &(_str->perm_lock) );

	string_store_cleanup( _str->stores );
}

STR_CTL *string_config_defaults( void )
{
	_str = (STR_CTL *) allocz( sizeof( STR_CTL ) );

	_str->perm = (BUF *) allocz( sizeof( BUF ) );
	_str->perm->sz = PERM_SPACE_BLOCK;

	pthread_mutex_init( &(_str->perm_lock), NULL );

	return _str;
}

int string_config_line( AVP *av )
{
	return -1;
}

