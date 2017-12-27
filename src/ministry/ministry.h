/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* ministry.h - main includes and global config                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_H
#define MINISTRY_H

#include "../shared/shared.h"

#ifndef Err
#define Err strerror( errno )
#endif

#ifndef VERSION_STRING
#define VERSION_STRING "unknown"
#endif


// crazy control
#include "typedefs.h"

// in order
#include "locks.h"
#include "net.h"
#include "udp.h"
#include "tcp.h"
#include "token.h"
#include "targets.h"
#include "data.h"
#include "gc.h"
#include "mem.h"
#include "synth.h"
#include "sort.h"
#include "stats.h"
#include "selfstats.h"



#ifdef DEBUG_SYNTH
#define debug_synth( ... )			debug( ## __VA_ARGS__ )
#else
#define debug_synth( ... )			(void) 0
#endif



// main control structure
struct ministry_control
{
	HTTP_CTL			*	http;
	PROC_CTL			*	proc;
	TGT_CTL				*	target;
	LOCK_CTL			*	locks;
	MEMT_CTL			*	mem;
	GC_CTL				*	gc;
	NET_CTL				*	net;
	STAT_CTL			*	stats;
	SYN_CTL				*	synth;
	TGTS_CTL			*	tgt;
};



// global control config
MIN_CTL *ctl;


#endif
