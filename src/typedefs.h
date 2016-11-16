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
typedef struct mem_control			MEM_CTL;
typedef struct network_control		NET_CTL;
typedef struct stats_control		STAT_CTL;
typedef struct synth_control		SYN_CTL;
typedef struct http_control			HTTP_CTL;
typedef struct target_control		TGT_CTL;

typedef struct net_in_port			NET_PORT;
typedef struct net_type				NET_TYPE;
typedef struct stat_config			ST_CFG;
typedef struct stat_thread_ctl		ST_THR;
typedef struct stat_threshold		ST_THOLD;
typedef struct stat_moments			ST_MOM;
typedef struct config_context		CCTXT;
typedef struct points_list			PTLIST;
typedef struct mem_type_blank		MTBLANK;
typedef struct ip_network			IPNET;
typedef struct ip_check				IPCHK;
typedef struct mem_type_control		MTYPE;
typedef struct data_hash_vals		DVAL;
typedef struct data_hash_entry		DHASH;
typedef struct data_type_params		DTYPE;
typedef struct io_buffer			IOBUF;
typedef struct io_buffer_list		IOLIST;
typedef struct net_socket			NSOCK;
typedef struct target_io_list		TGTIO;
typedef struct target_type_data		TTYPE;
typedef struct target_stype_data	TSTYPE;
typedef struct target_set			TSET;
typedef struct target_conf			TARGET;
typedef struct host_prefix			HPRFX;
typedef struct host_prefixes		HPRFXS;
typedef struct host_data			HOST;
typedef struct thread_data			THRD;
typedef struct dhash_locks			DLOCKS;
typedef struct synth_data			SYNTH;
typedef struct words_data			WORDS;
typedef struct string_buffer		BUF;
typedef struct av_pair				AVP;
typedef struct lockless_counter		LLCT;
typedef struct http_callbacks		HTTP_CB;
typedef struct http_ssl				SSL_CONF;
typedef struct http_ssl_file		SSL_FILE;
typedef struct token_data			TOKEN;
typedef struct token_info			TOKENS;
typedef struct regex_entry			RGX;
typedef struct regex_list			RGXL;


// function types
typedef void target_fn ( ST_THR *, BUF *, IOBUF * );
typedef int64_t io_fn( TARGET * );
typedef void tsf_fn ( ST_THR *, BUF * );
typedef void stats_fn ( ST_THR * );
typedef void loop_call_fn ( int64_t, void * );
typedef void * throw_fn ( void * );
typedef void add_fn ( char *, int, char * );
typedef void line_fn ( HOST *, char *, int );
typedef void synth_fn( SYNTH * );

#endif
