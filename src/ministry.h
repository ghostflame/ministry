#ifndef MINISTRY_H
#define MINISTRY_H

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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <sys/resource.h>


#ifndef Err
#define Err strerror( errno )
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
#include "io.h"
#include "log.h"
#include "data.h"
#include "gc.h"
#include "mem.h"
#include "stats.h"

// last
#include "config.h"



// global control config
MIN_CTL *ctl;


// functions from main
void shut_down( int exval );


#endif
