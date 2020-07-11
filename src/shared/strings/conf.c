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
	string_store_cleanup( _str->stores );
}

STR_CTL *string_config_defaults( void )
{
	_str = (STR_CTL *) mem_perm( sizeof( STR_CTL ) );
	return _str;
}

int string_config_line( AVP *av )
{
	return -1;
}

