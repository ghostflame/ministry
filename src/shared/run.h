/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* run.h - defines run flags and states                                    *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_RUN_H
#define SHARED_RUN_H

// run flags

// overall control
#define RUN_DAEMON					0x00000001
#define RUN_STATS					0x00000002
#define RUN_DEBUG					0x00000004
#define RUN_PIDFILE					0x00000008

#define RUN_LOOP					0x00000010
#define RUN_SHUTDOWN				0x00000020
#define RUN_LOOP_CTL_MASK			0x00000030

// turns off some behaviours
#define RUN_BY_HAND					0x00000080


// this one disables daemon
#define RUN_TGT_STDOUT				0x00000100
// curl can verify certs
#define RUN_CURL_VERIFY				0x00000200


// app progress
#define RUN_APP_INIT				0x00001000
#define RUN_APP_START				0x00002000
#define RUN_APP_READY				0x00004000

#define RUN_SHUT_STATS				0x00010000
#define RUN_SHUT_MASK				0x000f0000

// eliminate some code from some apps
#define RUN_NO_HTTP					0x00100000
#define RUN_NO_TARGET				0x00200000
#define RUN_NO_RKV					0x00400000

// runtime control
#define RUN_SILENT_OUT				0x01000000

#endif
