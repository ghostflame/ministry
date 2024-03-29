/**************************************************************************
* Copyright 2015 John Denholm                                             *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
*                                                                         *
*                                                                         *
* shared.h - includes to tie shared together                              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef SHARED_SHARED_H
#define SHARED_SHARED_H


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
#include <getopt.h>
#include <malloc.h>
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
#include <termios.h>
#include <features.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <curl/curl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <gnutls/x509.h>
#include <json-c/json.h>
#include <openssl/sha.h>
#include <sys/inotify.h>
#include <sys/resource.h>
#include <gnutls/gnutls.h>

#define MHD_PLATFORM_H
#include <microhttpd.h>

#ifndef Err
#define Err strerror( errno )
#endif


// makes it easier and remove some order-dependence
#include "typedefs.h"

#include "run.h"
#include "log/log.h"
#include "utils/loop.h"
#include "strings/stringutils.h"
#include "utils/utils.h"
#include "utils/iter.h"
#include "curlw.h"
#include "regexp.h"
#include "iplist/iplist.h"
#include "fs/fs.h"
#include "net/net.h"
#include "net/token.h"
#include "net/udp.h"
#include "mem/mem.h"
#include "rkv/rkv.h"
#include "target/target.h"
#include "io/io.h"
#include "thread.h"
#include "http/http.h"
#include "pmet/pmet.h"
#include "slack/slack.h"
#include "ha/ha.h"
#include "config/config.h"
#include "app.h"


// process control global
extern PROC_CTL *_proc;


#endif
