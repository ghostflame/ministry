/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
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
