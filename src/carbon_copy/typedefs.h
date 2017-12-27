/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* typedefs.h - major typedefs for other structures                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef CARBON_COPY_TYPEDEFS_H
#define CARBON_COPY_TYPEDEFS_H

typedef struct carbon_copy_control		RCTL;
typedef struct memt_control			MEMT_CTL;
typedef struct network_control		NET_CTL;
typedef struct relay_control		RLY_CTL;
typedef struct target_control		TGT_CTL;

typedef struct stat_config			ST_CFG;
typedef struct stat_thread_ctl		ST_THR;
typedef struct stat_threshold		ST_THOLD;
typedef struct net_in_port			NET_PORT;
typedef struct net_type				NET_TYPE;
typedef struct relay_rule			RELAY;
typedef struct relay_line			RLINE;
typedef struct host_prefix			HPRFX;
typedef struct host_prefixes		HPRFXS;
typedef struct host_data			HOST;
typedef struct host_buffers			HBUFS;
typedef struct dhash_locks			DLOCKS;

// function types
typedef int  relay_fn ( HBUFS *, RLINE * );
typedef void line_fn ( HOST *, char *, int );

#endif
