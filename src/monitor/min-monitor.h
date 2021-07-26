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
* monitor.h - main includes and global config                             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MONITOR_H
#define MONITOR_H

#include "../shared/shared.h"

// crazy control
#include "typedefs.h"

// in order - other headers
#include "mem.h"
#include "monitors.h"




struct monitor_control
{
	PROC_CTL			*	proc;
	MEMT_CTL			*	mem;
	MONCFG				*	mons;
};


// global control config
extern MOCTL *ctl;


#endif
