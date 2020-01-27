/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* query.h - query object structures
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef ARCHIVIST_QUERY_H
#define ARCHIVIST_QUERY_H



struct query_path
{
	QP					*	next;
	TEL					*	tel;
};


struct query_data
{
	QRY					*	next;
	char				*	search;
	QP					*	paths;

	int32_t					pcount;
	int16_t					slen;
	uint16_t				flags;

	int64_t					start;
	int64_t					end;

	char					partbuf[512];
};


int query_search( QRY *q );

void query_free( QRY *q );
QRY *query_create( char *search_str );


#endif
