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
* metric/metric.h - defines metric fns and structs                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef MINISTRY_TEST_METRIC_H
#define MINISTRY_TEST_METRIC_H


enum metric_type_vals
{
	METRIC_TYPE_ADDER = 0,
	METRIC_TYPE_STATS,
	METRIC_TYPE_GAUGE,
	METRIC_TYPE_HISTO,
	METRIC_TYPE_COMPAT,
	METRIC_TYPE_MAX
};



struct metric_control
{
	MGRP				*	groups;
	MGRP				*	flat_list;
	int64_t					gcount;
	int64_t					mcount;
	int64_t					max_age;
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

void metric_start_all( void );

MTRC_CTL *metric_config_defaults( void );
conf_line_fn metric_config_line;


#endif
