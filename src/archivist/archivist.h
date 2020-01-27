/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
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

// include epoll
#include <sys/epoll.h>


// crazy control
#include "typedefs.h"

// in order
#include "network.h"
#include "data.h"
#include "tree.h"
#include "query.h"


// main control structure
struct archivist_control
{
	HTTP_CTL			*	http;
	PROC_CTL			*	proc;
};



// global control config
RKV_CTL *ctl;


#endif
