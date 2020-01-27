/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* query.c - query functions                                               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "archivist.h"


int query_search( QRY *q )
{
	return 0;
}


QRY *query_create( char *search_string )
{
	QRY *q = malloc( sizeof( QRY ) );

	return q;
}

void query_free( QRY *q )
{
	return;
}
