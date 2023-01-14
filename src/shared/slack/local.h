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
* slack/local.h - private functions handling comms with slack             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#ifndef SHARED_SLACK_LOCAL_H
#define SHARED_SLACK_LOCAL_H


#define SLACK_CURL_FLAGS			CURLW_FLAG_SEND_JSON|CURLW_FLAG_VERBOSE|CURLW_FLAG_DEBUG|CURLW_FLAG_PARSE_JSON


#include "shared.h"


extern SLK_CTL *_slk;

#endif
