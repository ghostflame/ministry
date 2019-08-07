/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
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
typedef struct network_control		NET_CTL;
typedef struct gc_control			GC_CTL;
typedef struct stats_control		STAT_CTL;
typedef struct synth_control		SYN_CTL;
typedef struct http_control			HTTP_CTL;
typedef struct targets_control		TGTS_CTL;
typedef struct fetch_control		FTCH_CTL;
typedef struct metrics_control		MET_CTL;

//typedef struct net_in_port			NET_PORT;
//typedef struct net_type				NET_TYPE;
//typedef struct net_prefix			NET_PFX;
typedef struct stat_thread_ctl		ST_THR;
typedef struct stat_config			ST_CFG;
typedef struct stat_threshold		ST_THOLD;
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
typedef struct data_type_params		DTYPE;
typedef struct targets_type_data	TTYPE;
typedef struct targets_set			TSET;
typedef struct host_prefixes		HPRFXS;
//typedef struct host_data			HOST;
typedef struct synth_data			SYNTH;
typedef struct synth_fn_def			SYNDEF;
typedef struct token_data			TOKEN;
typedef struct token_info			TOKENS;
//typedef struct tcp_thread			TCPTH;
typedef struct fetch_target			FETCH;
typedef struct metrics_type			METTY;
typedef struct metrics_attr			METAT;
typedef struct metrics_attr_list	METAL;
typedef struct metrics_attr_map		METMP;
typedef struct metrics_profile		METPR;
typedef struct metrics_entry		METRY;
typedef struct metrics_data			MDATA;


// function types
typedef void targets_fn ( ST_THR *, BUF *, IOBUF * );
typedef void tsf_fn ( ST_THR *, BUF * );
typedef void stats_fn ( ST_THR * );
typedef void pred_fn ( ST_THR *, DHASH * );
//typedef void add_fn ( char *, int, char * );
//typedef void line_fn ( HOST *, char *, int );
typedef void synth_fn( SYNTH * );
//typedef void tcp_setup_fn ( NET_TYPE * );
//typedef void tcp_fn ( HOST * );


#endif
