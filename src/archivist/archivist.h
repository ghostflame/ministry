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
* archivist.h - main includes and global config                           *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef ARCHIVIST_H
#define ARCHIVIST_H

#include "../shared/shared.h"

#ifndef Err
#define Err strerror( errno )
#endif

// fnmatch for globbing
#include <fnmatch.h>

// crazy control
#include "typedefs.h"

// in order
#include "mem.h"
#include "network.h"
#include "data.h"
#include "query/query.h"


// main control structure
struct archivist_control
{
	HTTP_CTL			*	http;
	PROC_CTL			*	proc;
	MEMT_CTL			*	mem;
	QRY_CTL				*	query;
	NETW_CTL			*	netw;
	RKV_CTL				*	rkv;
};



// global control config
extern ARCH_CTL *ctl;


#endif
