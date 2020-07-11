/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* query/query.h - query object structures                                 *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef ARCHIVIST_QUERY_H
#define ARCHIVIST_QUERY_H


#define QUERY_PARAM_PATH		"path"
#define QUERY_PARAM_FROM		"from"
#define QUERY_PARAM_SPAN		"span"
#define QUERY_PARAM_TO			"to"
#define QUERY_PARAM_METRIC		"metric"


enum query_arg_types
{
	QARG_TYPE_NONE = 0,  // used to fill the structure
	QARG_TYPE_DOUBLE,
	QARG_TYPE_INTEGER,
	QARG_TYPE_STRING,
	QARG_TYPE_FUNCTION,
	QARG_TYPE_MAX
};


struct query_fn
{
	char				*	name;
	query_data_fn		*	fn;
	int8_t					argc;	// max args
	int8_t					argm;	// min args
	int8_t					types[4];
};

struct query_fn_call
{
	QRFC				*	next;
	QRFN				*	fn;
	int						argc;
	char				**	argv;
};


struct query_path
{
	QP					*	next;
	RKQR				*	fq;
	TEL					*	tel;
};


struct query_data
{
	QRY					*	next;
	char				*	search;
	QRFN				*	fns;		// innermost first
	QP					*	paths;

	int64_t					q_when;		// when it was asked for

	int32_t					pcount;
	int16_t					slen;
	uint16_t				flags;

	int64_t					start;
	int64_t					end;

	WORDS					w;
	int						plast;
	int						metric;

	char					partbuf[512];
};


struct query_control
{
	int64_t					default_timespan;
	int						max_paths;
};

extern QRY_CTL *_qry;


int query_search( QRY *q );
void query_free( QRY *q );
QRY *query_create( const char *search_str );

http_callback query_get_callback;

void query_init( void );

QRY_CTL *query_config_defaults( void );
conf_line_fn query_config_line;

#endif
