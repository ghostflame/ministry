#ifndef MINISTRY_TYPEDEFS_H
#define MINISTRY_TYPEDEFS_H

typedef struct ministry_control		MIN_CTL;
typedef struct log_control			LOG_CTL;
typedef struct lock_control			LOCK_CTL;
typedef struct data_control			DATA_CTL;
typedef struct memory_control		MEM_CTL;
typedef struct network_control		NET_CTL;
typedef struct stats_control		STAT_CTL;

typedef struct net_in_port			NET_PORT;
typedef struct net_type				NET_TYPE;
typedef struct config_context		CCTXT;
typedef struct points_list			PTLIST;
typedef struct data_stat_entry		DSTAT;
typedef struct data_add_entry		DADD;
typedef struct net_buffer			NBUF;
typedef struct net_socket			NSOCK;
typedef struct host_data			HOST;
typedef struct thread_data			THRD;
typedef struct words_data			WORDS;
typedef struct av_pair				AVP;

// function types
typedef void loop_call_fn ( void );
typedef void * throw_fn ( void * );
typedef void line_fn ( HOST *, char *, int );

#endif
