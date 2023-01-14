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
* relay.h - defines relay structures and functions                        * 
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#ifndef CARBON_COPY_RELAY_LOCAL_H
#define CARBON_COPY_RELAY_LOCAL_H


#define LINE_SEPARATOR				'\n'
#define RELAY_MAX_REGEXES			32

#define RELAY_FLUSH_MSEC			1000L	// in msec
#define RELAY_FLUSH_MSEC_FAST		25L		// for real-time stats
#define RELAY_FLUSH_MSEC_SLOW		5000L	// for data with timestamps in - no rush


#include "../carbon_copy.h"




struct relay_line
{
	char				*	line;
	int32_t					len;
	int16_t					plen;
	char					sep;
};



extern RLY_CTL *_relay;




#endif

