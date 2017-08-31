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

typedef struct mintest_control		MTEST_CTL;
typedef struct metric_control		MTRC_CTL;
typedef struct lock_control			LOCK_CTL;
typedef struct log_control			LOG_CTL;
typedef struct mem_control			MEM_CTL;
typedef struct target_control		TGT_CTL;

typedef struct config_section		CSECT;
typedef struct config_context		CCTXT;
typedef struct mem_type_blank		MTBLANK;
typedef struct mem_type_control		MTYPE;
typedef struct net_socket			NSOCK;
typedef struct io_buffer			IOBUF;
typedef struct io_buffer_list		IOLIST;
typedef struct target_io_list		TGTIO;
typedef struct target_conf			TARGET;
typedef struct thread_data			THRD;
typedef struct words_data			WORDS;
typedef struct string_buffer		BUF;
typedef struct av_pair				AVP;
typedef struct lockless_counter		LLCT;
typedef struct regex_entry			RGX;
typedef struct regex_list			RGXL;
typedef struct metric				METRIC;
typedef struct metric_group			MGRP;

// function types
typedef void loop_call_fn ( int64_t, void * );
typedef void * throw_fn ( void * );
typedef int conf_line_fn ( AVP * );

typedef int64_t io_fn( TARGET * );
typedef void update_fn ( METRIC * );
typedef void line_fn ( METRIC * );

#endif
