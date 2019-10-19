/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* typedefs.h - major typedefs for other structures                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_TYPEDEFS_H
#define SHARED_TYPEDEFS_H

typedef struct process_control      PROC_CTL;
typedef struct log_control          LOG_CTL;
typedef struct mem_control          MEM_CTL;
typedef struct http_control         HTTP_CTL;
typedef struct iplist_control       IPL_CTL;
typedef struct io_control           IO_CTL;
typedef struct target_control       TGT_CTL;
typedef struct ha_control           HA_CTL;
typedef struct pmet_control         PMET_CTL;
typedef struct net_control          NET_CTL;

typedef struct config_section       CSECT;
typedef struct config_context       CCTXT;
typedef struct config_file          CFILE;

typedef struct log_file             LOGFL;
typedef struct log_setdebug         LOGSD;

typedef struct mem_call_counters    MCCTR;
typedef struct mem_type_counters    MTCTR;
typedef struct mem_type_stats       MTSTAT;
typedef struct mem_type_blank       MTBLANK;
typedef struct mem_type             MTYPE;
typedef struct mem_check            MCHK;
typedef struct mem_hanger           MEMHG;
typedef struct mem_hanger_list      MEMHL;

typedef struct iterator             ITER;

typedef struct io_buffer            IOBUF;
typedef struct io_socket            SOCK;
typedef struct io_buf_ptr           IOBP;

typedef struct target               TGT;
typedef struct target_metrics       TGTMT;
typedef struct target_list          TGTL;
typedef struct target_alter         TGTALT;

typedef struct iplist_net           IPNET;
typedef struct iplist               IPLIST;

typedef struct host_tracker         HTRACK;
typedef struct host_data            HOST;
typedef struct tcp_thread           TCPTH;

typedef struct net_prefix           NET_PFX;
typedef struct net_type             NET_TYPE;
typedef struct net_in_port          NET_PORT;

typedef struct token_data			TOKEN;
typedef struct token_info			TOKENS;

typedef struct thread_data          THRD;
typedef struct av_pair              AVP;
typedef struct lockless_counter     LLCT;
typedef struct regex_entry          RGX;
typedef struct regex_list           RGXL;

typedef struct words_data           WORDS;
typedef struct string_buffer        BUF;
typedef struct string_store_entry   SSTE;
typedef struct string_store         SSTR;

typedef struct http_path            HTPATH;
typedef struct http_handlers        HTHDLS;
typedef struct http_req_data        HTREQ;
typedef struct http_post_state      HTTP_POST;

typedef struct http_callbacks       HTTP_CB;
typedef struct http_tls             TLS_CONF;
typedef struct http_tls_file        TLS_FILE;

typedef struct json_object          JSON;

typedef struct curlw_container      CURLWC;
typedef struct curlw_times          CURLWT;
typedef struct curlw_handle         CURLWH;

typedef struct ha_partner           HAPT;

typedef struct pmet_source          PMETS;
typedef struct pmet_metric          PMETM;
typedef struct pmet_item            PMET;
typedef struct pmet_label           PMET_LBL;
typedef struct pmet_shared          PMET_SH;


// function types
typedef void loop_call_fn ( int64_t, void * );
typedef void throw_fn ( THRD * );
typedef int conf_line_fn ( AVP * );
typedef int http_callback ( HTREQ * );
typedef int json_callback ( json_object * );
typedef int http_reporter ( HTPATH *, void *arg, int64_t, int64_t );
typedef void help_fn ( void );
typedef int64_t io_fn( TGT * );
typedef int target_cfg_fn( TGT *, char *, int );
typedef void curlw_cb( void *, IOBUF *b );
typedef void curlw_jcb( void *, json_object *jso );
typedef int sort_fn( const void *, const void * );
typedef double pmet_gen_fn( int64_t, void *, double * );
typedef void iplist_data_fn( void *, IPNET * );

// found in the apps
typedef int  buf_fn ( HOST *, IOBUF * );
typedef void line_fn ( HOST *, char *, int );
typedef void add_fn ( char *, int, char * );
typedef void tcp_setup_fn ( NET_TYPE * );
typedef void tcp_fn ( HOST * );

#endif
