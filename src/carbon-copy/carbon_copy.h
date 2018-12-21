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
#include "net.h"
#include "udp.h"
#include "tcp.h"
#include "mem.h"
#include "target.h"
#include "hash.h"
#include "relay.h"
#include "selfstats.h"


struct carbon_copy_control
{
	PROC_CTL			*	proc;
	LOG_CTL				*	log;
	RLY_CTL				*	relay;
	MEMT_CTL			*	mem;
	NET_CTL				*	net;
	TGT_CTL				*	target;
};



// global control config
RCTL *ctl;


#endif
