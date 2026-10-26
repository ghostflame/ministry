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
#define QUERY_PARAM_DEBUG		"debug"


#define QUERY_FLAGS_DEBUG		0x0001


enum query_arg_types
{
	QARG_TYPE_NONE = 0,  // used to fill the structure
	QARG_TYPE_DOUBLE,
	QARG_TYPE_INTEGER,
	QARG_TYPE_STRING,
	QARG_TYPE_FUNCTION,
	QARG_TYPE_SERIES,
	QARG_TYPE_MULTI,
	QARG_TYPE_MAX
};

#define QARG_MAX			4

struct query_fn
{
	char				*	name;
	query_data_fn		*	fn;
	int8_t					nlen;
	int8_t					argc;	// max args
	int8_t					argm;	// min args
	int8_t					types[QARG_MAX];
};

union query_arg_val
{
	double					avd;
	int						avi;
	char				*	avs;
};

struct query_arg
{
	QARG				*	next;
	int						type;
	int						len;
	QVAL					val;
};

struct query_fn_call
{
	QRFC				*	next;
	QRY					*	qry;
	QRFN				*	fn;		// definition
	PTL					*	in;		// linked list of points lists
	PTL					*	out;	// linked list of treated lists
	int						argc;
	QARG					args[QARG_MAX];
};

struct query_parse_res
{
	QPRES				*	next;
	QRY					*	qry;
	QRFC				*	fn;
	char				*	in;
	char				*	fpar;
	char				*	bpar;
	int16_t					inlen;
	int16_t					flen;
	int16_t					blen;
	char					got_args;
	char					got_nest;
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
	QRFC				*	fns;		// innermost first
	QP					*	paths;

	int64_t					q_when;		// when it was asked for

	int32_t					pcount;

	int16_t					slen;
	uint16_t				flags;

	int64_t					start;
	int64_t					end;

	WORDS					w;
	int						plast;
	int						metric;		// RKV_VAL_METR_*

	char					partbuf[512];
};



// nested fn call structure
struct query_layer
{
	QLYR				*	next;		// for peers
	QLYR				*	child;		// for inner layers
	QLYR				*	parent;		// NULL only at top layer
	QRY					*	q;			// link up to the query
	QP					*	paths;
	QRFC				*	fn;			// function and args
	char				*	start;
	char				*	end;
	char				*	b_open;
	char				*	b_close;
};


struct query_data_new
{
	QRY					*	next;
	char				*	search;
	QLYR				*	layers;		// per fn (might have their own path lookups)

	int16_t					slen;
	uint16_t				flags;
	int32_t					metric;		// display metrics

	int64_t					start;
	int64_t					end;

	int64_t					q_when;		// when it was asked for

	char					workbuf[512];
};



struct query_metrics
{
	PMETS				*	source;
	PMETM				*	queries;
	PMETM				*	current;
	PMETM				*	timings;
	PMET				*	pm_qry;
	PMET				*	pm_cur;
	PMET				*	pm_tmg;
};


struct query_control
{
	QRMET				*	metrics;

	int64_t					default_timespan;
	int						max_paths;

	int64_t					count;
	int						max_curr;
	int						curr;

	pthread_mutex_t			qlock;
	int						use_lock;
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
