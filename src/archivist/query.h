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


#define DEFAULT_QUERY_TIMESPAN			3600000000		// 1hr
#define DEFAULT_QUERY_MAX_PATHS			2500


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

	WORDS					w;
	int						plast;

	char					partbuf[512];
};


struct query_control
{
	int64_t					default_timespan;
	int						max_paths;
};

int query_search( QRY *q );

void query_free( QRY *q );
QRY *query_create( char *search_str );

QRY_CTL *query_config_defaults( void );
conf_line_fn query_config_line;

#endif
