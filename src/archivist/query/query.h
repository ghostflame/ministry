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
