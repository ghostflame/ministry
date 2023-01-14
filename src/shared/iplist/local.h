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
