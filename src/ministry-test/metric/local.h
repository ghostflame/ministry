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
* metric/local.h - defines metric-specific fns and structs                *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef MINISTRY_TEST_METRIC_LOCAL_H
#define MINISTRY_TEST_METRIC_LOCAL_H


#define METRIC_MAX_AGE				100			// msec
#define METRIC_DEFAULT_UPD_INTV		10000000	// usec
#define METRIC_DEFAULT_REP_INTV		10000000	// usec
#define METRIC_MAX_PATH				1024		// bytes
#define METRIC_WTMP_SZ				4096		// bytes


// random number generator
#define metrand( )					( (double) random( ) / ( 1.0 * RAND_MAX ) )
#define metrandM( )					( -0.5 + metrand( ) )



#include "ministry_test.h"

typedef struct metric_model_data    MODEL;



struct metric_model_data
{
	const char			*	name;
	update_fn			*	updater;
	int8_t					model;
};


extern const MODEL metric_types[];


enum metric_model_vals
{
	METRIC_MODEL_TRACK_MEAN = 0,
	METRIC_MODEL_TRACK_MEAN_POS,
	METRIC_MODEL_FLOOR_UP,
	METRIC_MODEL_SMTS_TRACK,
	METRIC_MODEL_MAX
};

enum metric_fields
{
	METRIC_FLD_TYPE = 0,
	METRIC_FLD_MODEL,
	METRIC_FLD_D1,
	METRIC_FLD_D2,
	METRIC_FLD_D3,
	METRIC_FLD_D4,
	METRIC_FLD_PATH,
	METRIC_FLD_MAX
};


// either in groups, or individual stats
struct metric
{
	METRIC				*	next;
	MGRP				*	grp;
	BUF					*	path;
	update_fn			*	ufp;
	char				*	trlr;
	const MODEL			*	model;

	double					curr;
	double					d1;
	double					d2;
	double					d3;
	double					d4;

	int64_t					intv;

	int8_t					type;
	int8_t					tlen;
	char					sep;
};


#define lock_mgrp( m )			pthread_mutex_lock(   &(m->lock) )
#define unlock_mgrp( m )		pthread_mutex_unlock( &(m->lock) )

struct metric_group
{
	MGRP				*	next;
	MGRP				*	next_flat;
	MGRP				*	parent;
	MGRP				*	children;
	METRIC				*	list;
	TGT					*	target;
	IOBUF				*	buf;
	char				*	name;
	BUF					*	prefix;
	char				*	wtmp;

	pthread_mutex_t			lock;
	int64_t					mcount;
	int64_t					upd_intv;
	int64_t					rep_intv;
	int64_t					repeat;
	int16_t					nlen;
	int8_t					closed;
	int8_t					tgttype;
};





update_fn metric_update_track_mean;
update_fn metric_update_track_mean_pos;
update_fn metric_update_floor_up;
update_fn metric_update_sometimes_track;

loop_call_fn metric_group_update;
loop_call_fn metric_group_report;
loop_call_fn metric_group_io;

throw_fn metric_group_update_loop;
throw_fn metric_group_report_loop;
throw_fn metric_group_io_loop;

// const.c
const MODEL *metric_get_model( const char *name );
int metric_get_type( const char *name );

#endif
