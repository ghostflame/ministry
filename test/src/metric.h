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


// either in groups, or individual stats
struct metric
{
	METRIC				*	next;
	MGRP				*	grp;
	BUF					*	path;
	update_fn			*	fp;

	double					curr;
	double					d1;
	double					d2;
	double					d3;
	double					d4;

	int64_t					intv;
	int64_t					persec;

	int64_t					use_lock;
};


struct metric_group
{
	MGRP				*	next;
	METRIC				*	list;
	TARGET				*	target;
	IOBUF				*	buf;
	char				*	name;
	BUF					*	prefix;

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

loop_call_fn metric_update;
loop_call_fn metric_group_update;

throw_fn metric_stat_loop;
throw_fn metric_group_loop;

void metric_start_all( void );

MTRC_CTL *metric_config_defaults( void );
int metric_config_line( AVP *av );


#endif
