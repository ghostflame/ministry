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

// fnmatch for globbing
#include <fnmatch.h>

// crazy control
#include "typedefs.h"

// in order
#include "mem.h"
#include "network.h"
#include "data.h"
#include "tree.h"
#include "query/query.h"
#include "file/file.h"


// main control structure
struct archivist_control
{
	HTTP_CTL			*	http;
	PROC_CTL			*	proc;
	MEMT_CTL			*	mem;
	TREE_CTL			*	tree;
	QRY_CTL				*	query;
	NETW_CTL			*	netw;
	FILE_CTL			*	file;
};



// global control config
RKV_CTL *ctl;


#endif
