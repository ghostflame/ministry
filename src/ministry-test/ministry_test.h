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
* ministry_test.h - main includes and global config                       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_TEST_H
#define MINISTRY_TEST_H

#include "../shared/shared.h"

// crazy control
#include "typedefs.h"

// in order
#include "metric/metric.h"
#include "targets.h"


// max mem - 3G
#define DEFAULT_MT_MAX_KB		( 3 * 1024 * 1024 )


// main control structure
struct mintest_control
{
	MTRC_CTL			*	metric;
	TGTS_CTL			*	tgt;
	PROC_CTL			*	proc;
	TGT_CTL				*	target;
	LOG_CTL				*	log;
};


// global control config
extern MTEST_CTL *ctl;


#endif
