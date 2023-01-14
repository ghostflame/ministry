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
* metric_filter.h - main includes and global config                       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef METRIC_FILTER_H
#define METRIC_FILTER_H

#include "../shared/shared.h"

// crazy control
#include "typedefs.h"

// in order
#include "mem.h"
#include "network.h"
#include "filter/filter.h"


struct metric_filter_control
{
	PROC_CTL			*	proc;
	MEMT_CTL			*	mem;
	FLT_CTL				*	filt;
	NETW_CTL			*	netw;
};


// global control config
extern MCTL *ctl;


#endif
