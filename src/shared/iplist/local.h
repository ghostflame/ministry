/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* iplist/local.h - defines network ip filtering structures                *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_IPLIST_LOCAL_H
#define SHARED_IPLIST_LOCAL_H

#define IPLIST_REGEX_OCT	"(25[0-5]|2[0-4][0-9]||1[0-9][0-9]|[1-9][0-9]|[0-9])"
#define IPLIST_REGEX_NET	"^((" IPLIST_REGEX_OCT "\\.){3}" IPLIST_REGEX_OCT ")(/(3[0-2]|[12]?[0-9]))?$"

#define IPLIST_HASHSZ		2003

#define IPLIST_LOCALONLY	"localhost-only"

#define lock_iplists( )		pthread_mutex_lock(   &(_iplist->lock) )
#define unlock_iplists( )	pthread_mutex_unlock( &(_iplist->lock) )



#include "shared.h"


extern IPL_CTL *_iplist;


#endif
