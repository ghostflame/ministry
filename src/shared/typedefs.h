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

typedef struct config_section       CSECT;
typedef struct config_context       CCTXT;
typedef struct mem_type_blank       MTBLANK;
typedef struct mem_type             MTYPE;
typedef struct mem_type_stats       MTSTAT;

typedef struct io_buffer            IOBUF;
typedef struct io_socket            SOCK;
typedef struct io_buf_ptr           IOBP;

typedef struct target               TGT;
typedef struct target_list          TGTL;

typedef struct iplist_net           IPNET;
typedef struct iplist               IPLIST;

typedef struct thread_data          THRD;
typedef struct words_data           WORDS;
typedef struct string_buffer        BUF;
typedef struct av_pair              AVP;
typedef struct lockless_counter     LLCT;
typedef struct regex_entry          RGX;
typedef struct regex_list           RGXL;

typedef struct http_path            HTPATH;
typedef struct http_callbacks       HTTP_CB;
typedef struct http_ssl             SSL_CONF;
typedef struct http_ssl_file        SSL_FILE;



// function types
typedef void loop_call_fn ( int64_t, void * );
typedef void * throw_fn ( void * );
typedef int conf_line_fn ( AVP * );
typedef int http_handler ( uint32_t, char **, int, void * );
typedef void help_fn ( void );
typedef int64_t io_fn( TGT * );
typedef int target_cfg_fn( TGT *, char *, int );

typedef void iplist_data_fn( void *, IPNET * );

#endif