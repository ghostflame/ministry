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

#define _GNU_SOURCE

// here's what we need in addition
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

#define MHD_PLATFORM_H
#include <microhttpd.h>

#ifndef Err
#define Err strerror( errno )
#endif

#ifndef MINISTRY_VERSION
#define MINISTRY_VERSION "unknown"
#endif


// crazy control
#include "typedefs.h"

// not in order
#include "run.h"
#include "loop.h"
#include "thread.h"

// in order
#include "utils.h"
#include "net.h"
#include "udp.h"
#include "tcp.h"
#include "io.h"
#include "token.h"
#include "target.h"
#include "log.h"
#include "data.h"
#include "gc.h"
#include "mem.h"
#include "synth.h"
#include "stats.h"
#include "http.h"
#include "selfstats.h"

// last
#include "config.h"



// global control config
MIN_CTL *ctl;


// functions from main
void shut_down( int exval );


#endif
