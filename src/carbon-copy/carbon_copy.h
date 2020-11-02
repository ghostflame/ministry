/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
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
