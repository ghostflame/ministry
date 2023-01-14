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
typedef struct slack_control		SLK_CTL;
typedef struct string_control       STR_CTL;
typedef struct rkv_control          RKV_CTL;

typedef struct config_section       CSECT;
typedef struct config_context       CCTXT;

typedef struct data_point			PNT;
typedef struct data_series			PTL;

typedef struct log_file             LOGFL;
typedef struct log_setdebug         LOGSD;

typedef struct file_watch           FWTCH;
typedef struct file_tree            FTREE;

typedef struct mem_call_counters    MCCTR;
typedef struct mem_type_counters    MTCTR;
typedef struct mem_type_stats       MTSTAT;
typedef struct mem_type_blank       MTBLANK;
typedef struct mem_type             MTYPE;
typedef struct mem_check            MCHK;
typedef struct mem_hanger           MEMHG;
typedef struct mem_hanger_list      MEMHL;
typedef struct mem_perm             PERM;

typedef struct iterator             ITER;

typedef struct io_buffer            IOBUF;
typedef struct io_socket            SOCK;
typedef struct io_buf_ptr           IOBP;
typedef struct io_tls               IOTLS;

typedef struct target               TGT;
typedef struct target_metrics       TGTMT;
typedef struct target_list          TGTL;

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
typedef struct http_user            HTUSR;
typedef struct http_req_data        HTREQ;
typedef struct http_post_state      HTTP_POST;
typedef struct http_param           HTPRM;

typedef struct http_callbacks       HTTP_CB;
typedef struct http_tls             TLS_CONF;
typedef struct http_tls_file        TLS_FILE;

typedef struct json_object          JSON;

typedef struct curlw_times          CURLWT;
typedef struct curlw_handle         CURLWH;

typedef struct ha_partner           HAPT;

typedef struct pmet_source          PMETS;
typedef struct pmet_metric          PMETM;
typedef struct pmet_item            PMET;
typedef struct pmet_label           PMET_LBL;
typedef struct pmet_shared          PMET_SH;

typedef struct slack_message        SLKMSG;
typedef struct slack_pathway        SLKPT;
typedef struct slack_handle         SLKHD;
typedef struct slack_channel        SLKCH;
typedef struct slack_space          SLKSP;

typedef struct rkv_query            RKQR;
typedef struct rkv_agg_entry        PNTA;
typedef struct rkv_header_start     RKSTT;
typedef struct rkv_bucket           RKBKT;
typedef struct rkv_bucket_match     RKBMT;
typedef struct rkv_header           RKHDR;
typedef struct rkv_metrics          RKMET;
typedef struct rkv_data             RKFL;
typedef struct rkv_file_thread      RKFT;
typedef struct rkv_tree_element     TEL;
typedef struct rkv_tree_leaf        LEAF;


// function types
typedef void loop_call_fn ( int64_t, void * );
typedef void throw_fn ( THRD * );
typedef int conf_line_fn ( AVP * );
typedef int http_callback ( HTREQ * );
typedef int json_callback ( JSON * );
typedef int ftree_callback ( FTREE *, uint32_t, const char *, const char *, void * );
typedef int http_reporter ( HTPATH *, void *, int64_t, int64_t );
typedef int store_callback( SSTE *e, void *arg );
typedef void help_fn ( void );
typedef int64_t io_fn( TGT * );
typedef int target_cfg_fn( TGT *, char *, int );
typedef void curlw_cb( void *, IOBUF * );
typedef void curlw_jcb( void *, JSON * );
typedef int sort_fn( const void *, const void * );
typedef double pmet_gen_fn( int64_t, void *, double * );
typedef void iplist_data_fn( void *, IPNET * );
typedef void mem_free_cb( void * );
// return 0 for filter 'true'
typedef int mhl_callback( MEMHL *, void *, MEMHG *, void * );
typedef void rkv_rd_fn ( RKQR * );

// found in the apps
typedef int  buf_fn ( HOST *, IOBUF * );
typedef void line_fn ( HOST *, char *, int );
typedef void add_fn ( const char *, int, const char * );
typedef void tcp_setup_fn ( NET_TYPE * );
typedef void tcp_fn ( HOST * );

#endif
