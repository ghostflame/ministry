/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
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
MTEST_CTL *ctl;


#endif
