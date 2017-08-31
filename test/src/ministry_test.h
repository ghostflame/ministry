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

#define _GNU_SOURCE

// here's what we need in addition
#include <math.h>
#include <poll.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <regex.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <features.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <sys/resource.h>

#ifndef Err
#define Err strerror( errno )
#endif

#ifndef VERSION_STRING
#define VERSION_STRING "unknown"
#endif


// crazy control
#include "typedefs.h"

// not in order
#include "run.h"
#include "loop.h"
#include "thread.h"
#include "regexp.h"

// in order
#include "utils.h"
#include "net.h"
#include "io.h"
#include "target.h"
#include "log.h"
#include "metric.h"
#include "mem.h"

// last
#include "config.h"


struct mintest_control
{
	PROC_CTL			*	proc;
	LOCK_CTL			*	locks;
	LOG_CTL				*	log;
	MEM_CTL				*	mem;
	MTRC_CTL			*	metric;
	TGT_CTL				*	tgt;
};


// global control config
MTEST_CTL *ctl;


// functions from main
void shut_down( int exval );


#endif
