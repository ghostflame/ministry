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
* http/unused.c - callbacks which are not currently used                  *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"



ssize_t http_unused_reader( void *cls, uint64_t pos, char *buf, size_t max )
{
	hinfo( "Called: http_unused_reader." );
	return -1;
}

void http_unused_reader_free( void *cls )
{
	hinfo( "Called: http_unused_reader_free." );
	return;
}


