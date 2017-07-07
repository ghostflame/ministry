/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* data.h - defines data simulator structures                              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef MINISTRY_TEST_DATA_H
#define MINISTRY_TEST_DATA_H


#define METRIC_MAX_AGE				50		// msec


struct metric_types_data
{
	const char			*	name;
	update_fn			*	updater;
	setup_fn			*	setup;
	int8_t					type;
};

enum metric_type_vals
{
	METRIC_TYPE_TRACK_MEAN = 0,
	METRIC_TYPE_FLOOR_UP,
	METRIC_TYPE_SMTS_TRACK,
	METRIC_TYPE_MAX
};

enum metric_fields
{
	METRIC_FLD_INTV = 0,
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
	setup_fn			*	sfp;

	double					curr;
	double					d1;
	double					d2;
	double					d3;
	double					d4;

	int64_t					intv;
	int64_t					persec;

	int8_t					use_lock;
	int8_t					type;
};


struct metric_group
{
	MGRP				*	next;
	METRIC				*	list;
	TARGET				*	target;
	IOBUF				*	buf;
	char				*	name;
	BUF					*	prefix;
	char				*	tgtstr;

	pthread_mutex_t			lock;
	int64_t					mcount;
	int64_t					intv;
	int64_t					as_group;
};


struct metric_control
{
	MGRP				*	groups;
	int64_t					gcount;
	int64_t					max_age;
};



update_fn metric_update_track_mean;
update_fn metric_update_floor_up;
update_fn metric_update_sometimes_track;

setup_fn metric_setup_track_mean;

loop_call_fn metric_update;
loop_call_fn metric_group_update;

throw_fn metric_stat_loop;
throw_fn metric_group_loop;

void metric_start_all( void );

MTRC_CTL *metric_config_defaults( void );
int metric_config_line( AVP *av );


#endif
