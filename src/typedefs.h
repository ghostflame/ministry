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
typedef struct log_control			LOG_CTL;
typedef struct lock_control			LOCK_CTL;
typedef struct memory_control		MEM_CTL;
typedef struct network_control		NET_CTL;
typedef struct stats_control		STAT_CTL;

typedef struct net_in_port			NET_PORT;
typedef struct net_type				NET_TYPE;
typedef struct stat_config			ST_CFG;
typedef struct stat_thread_ctl		ST_THR;
typedef struct config_context		CCTXT;
typedef struct points_list			PTLIST;
typedef union  data_hash_vals		DVAL;
typedef struct data_hash_entry		DHASH;
typedef struct io_buffer			IOBUF;
typedef struct net_socket			NSOCK;
typedef struct net_target			TARGET;
typedef struct host_data			HOST;
typedef struct thread_data			THRD;
typedef struct words_data			WORDS;
typedef struct av_pair				AVP;

// function types
typedef void loop_call_fn ( unsigned long long, void * );
typedef void * throw_fn ( void * );
typedef void line_fn ( HOST *, char *, int );

#endif
