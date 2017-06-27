/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* run.h - defines run flags and states                                    *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_TEST_RUN_H
#define MINISTRY_TEST_RUN_H

// run flags

#define RUN_DAEMON					0x00000001
#define RUN_STATS					0x00000002
#define RUN_DEBUG					0x00000004

#define RUN_LOOP					0x00000010
#define RUN_SHUTDOWN				0x00000020
#define RUN_LOOP_CTL_MASK			0x00000030

// this one disables daemon
#define RUN_TGT_STDOUT				0x00000100

#define RUN_SHUT_STATS				0x00010000
#define RUN_SHUT_MASK				0x000f0000

#endif
