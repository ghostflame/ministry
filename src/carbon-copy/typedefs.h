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

#ifndef CARBON_COPY_TYPEDEFS_H
#define CARBON_COPY_TYPEDEFS_H

typedef struct carbon_copy_control	RCTL;
typedef struct memt_control			MEMT_CTL;
typedef struct network_control		NETW_CTL;
typedef struct relay_control		RLY_CTL;
typedef struct target_control		TGT_CTL;
typedef struct selfstats_control    SST_CTL;

typedef struct stat_config			ST_CFG;
typedef struct stat_thread_ctl		ST_THR;
typedef struct stat_threshold		ST_THOLD;
typedef struct relay_data           RDATA;
typedef struct relay_rule			RELAY;
typedef struct relay_line			RLINE;
typedef struct host_prefix			HPRFX;
typedef struct host_prefixes		HPRFXS;
typedef struct host_buffers			HBUFS;
typedef struct dhash_locks			DLOCKS;

// function types
typedef int  relay_fn ( HBUFS *, RLINE * );
typedef uint32_t hash_fn ( const char *, int32_t );

#endif
