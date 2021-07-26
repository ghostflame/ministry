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
* stats.h - defines stats structures and routines                         *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_STATS_LOCAL_H
#define MINISTRY_STATS_LOCAL_H


#define st_thr_time( _name )		clock_gettime( CLOCK_REALTIME, &(t->_name) )



#include "ministry.h"






pred_fn stats_predict_linear;

stats_fn stats_stats_pass;
stats_fn stats_adder_pass;
stats_fn stats_gauge_pass;
stats_fn stats_histo_pass;
stats_fn stats_self_stats_pass;



#define DEFAULT_STATS_THREADS		6
#define DEFAULT_ADDER_THREADS		2
#define DEFAULT_GAUGE_THREADS		2
#define DEFAULT_HISTO_THREADS       2

#define DEFAULT_STATS_MSEC			10000

#define DEFAULT_STATS_PREFIX		"stats.timers."
#define DEFAULT_ADDER_PREFIX		""
#define DEFAULT_GAUGE_PREFIX		""
#define DEFAULT_HISTO_PREFIX        "stats.histograms."
#define DEFAULT_SELF_PREFIX			"self.ministry."

#define DEFAULT_MOM_MIN				30L
#define DEFAULT_MODE_MIN			30L

#define TSBUF_SZ					32
#define PREFIX_SZ					512
#define PATH_SZ						8192

#define MAX_HISTCF_COUNT			64



struct stat_threshold
{
	ST_THOLD		*	next;
	int					val;
	int					max;
	char			*	label;
};



// render
void bprintf( ST_THR *t, char *fmt, ... );

// utils
void stats_prefix( ST_CFG *c, char *s );
void stats_set_workspace( ST_THR *t, int32_t len );
void stats_set_bufs( ST_THR *t, ST_CFG *c, int64_t tval );

// self
void stats_thread_report( ST_THR *t );

#endif
