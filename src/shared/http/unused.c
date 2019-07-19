/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
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


