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
* target.h - defines network targets                                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_TARGET_LOCAL_H
#define SHARED_TARGET_LOCAL_H


#define TARGET_PROTO_TCP		IPPROTO_TCP
#define TARGET_PROTO_UDP		IPPROTO_UDP



#include "shared.h"


extern TGT_CTL *_tgt;


#endif
