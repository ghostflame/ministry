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
* typedefs.h - major typedefs for other structures                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_TYPEDEFS_H
#define MINISTRY_TYPEDEFS_H

typedef struct ministry_control		MIN_CTL;
typedef struct lock_control			LOCK_CTL;
typedef struct memt_control			MEMT_CTL;
typedef struct network_control		NETW_CTL;
typedef struct gc_control			GC_CTL;
typedef struct stats_control		STAT_CTL;
typedef struct synth_control		SYN_CTL;
typedef struct http_control			HTTP_CTL;
typedef struct targets_control		TGTS_CTL;
typedef struct fetch_control		FTCH_CTL;
typedef struct metrics_control		MET_CTL;

typedef struct stat_thread_ctl		ST_THR;
typedef struct stat_config			ST_CFG;
typedef struct stat_threshold		ST_THOLD;
typedef struct stat_hist_conf       ST_HIST;
typedef struct stat_moments			ST_MOM;
typedef struct stat_predict_conf	ST_PRED;
typedef struct stats_metrics        ST_MET;
typedef struct maths_prediction		PRED;
typedef struct maths_moments		MOMS;
typedef struct history_data_point	DPT;
typedef struct history				HIST;
typedef struct points_list			PTLIST;
typedef struct data_hash_vals		DVAL;
typedef struct data_hash_entry		DHASH;
typedef struct data_histogram       DHIST;
typedef struct data_type_params		DTYPE;
typedef struct targets_type_data	TTYPE;
typedef struct targets_set			TSET;
typedef struct host_prefixes		HPRFXS;
typedef struct synth_data			SYNTH;
typedef struct synth_fn_def			SYNDEF;
typedef struct fetch_target			FETCH;
typedef struct metrics_entry		METRY;
typedef struct metrics_data			MDATA;


// function types
typedef void targets_fn ( ST_THR *, BUF *, IOBUF * );
typedef void tsf_fn ( ST_THR *, BUF * );
typedef void stats_fn ( ST_THR * );
typedef void pred_fn ( ST_THR *, DHASH * );
typedef void dupd_fn ( DHASH *, double, char );
typedef void synth_fn( SYNTH * );


#endif
