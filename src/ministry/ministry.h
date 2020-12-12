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
* ministry.h - main includes and global config                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_H
#define MINISTRY_H

#include "../shared/shared.h"


// crazy control
#include "typedefs.h"

// in order
#include "locks.h"
#include "network.h"
#include "fetch/fetch.h"
#include "post.h"
#include "targets.h"
#include "data/data.h"
#include "metrics/metrics.h"
#include "gc.h"
#include "mem.h"
#include "synth/synth.h"
#include "maths/maths.h"
#include "stats/stats.h"




// main control structure
struct ministry_control
{
	HTTP_CTL			*	http;
	PROC_CTL			*	proc;
	TGT_CTL				*	target;
	LOCK_CTL			*	locks;
	MEMT_CTL			*	mem;
	GC_CTL				*	gc;
	STAT_CTL			*	stats;
	SYN_CTL				*	synth;
	TGTS_CTL			*	tgt;
	FTCH_CTL			*	fetch;
	MET_CTL				*	metric;
	NETW_CTL			*	net;
};



// global control config
extern MIN_CTL *ctl;


#endif
