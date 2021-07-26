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
* carbon_copy.h - main includes and global config                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef CARBON_COPY_H
#define CARBON_COPY_H

#include "../shared/shared.h"

// crazy control
#include "typedefs.h"

// in order
#include "mem.h"
#include "hash.h"
#include "host.h"
#include "relay/relay.h"
#include "selfstats.h"


#define CC_DEFAULT_HTTP_PORT			9081
#define CC_DEFAULT_HTTPS_PORT			9444


struct carbon_copy_control
{
	PROC_CTL			*	proc;
	RLY_CTL				*	relay;
	MEMT_CTL			*	mem;
	NETW_CTL			*	net;
	TGT_CTL				*	target;
	SST_CTL				*	stats;
};



// global control config
extern RCTL *ctl;


#endif
