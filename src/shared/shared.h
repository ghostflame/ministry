/*************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* shared.h - includes to tie shared together                              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef APP_SHARED_H
#define APP_SHARED_H


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
#include <dirent.h>
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

// makes it easier and remove some order-dependence
#include "typedefs.h"

#include "run.h"
#include "log.h"
#include "loop.h"
#include "utils.h"
#include "regexp.h"
#include "net.h"
#include "iplist.h"
#include "mem.h"
#include "target.h"
#include "io.h"
#include "thread.h"
#include "http.h"
#include "config.h"
#include "app.h"


// some globals
extern LOG_CTL *_logger;
extern MEM_CTL *_mem;
extern PROC_CTL *_proc;
extern HTTP_CTL *_http;
extern IPL_CTL *_iplist;
extern IO_CTL *_io;
extern TGT_CTL *_tgt;


#endif