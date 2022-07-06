/**************************************************************************
* Copyright 2015 John Denholm                                             *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
*                                                                         *
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

