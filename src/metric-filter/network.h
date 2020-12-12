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
* network.h - defines network functions and defaults                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef METRIC_FILTER_NETWORK_H
#define METRIC_FILTER_NETWORK_H

#define DEFAULT_METFILT_PORT		2030

#define MF_DEFAULT_HTTP_PORT		9082
#define MF_DEFAULT_HTTPS_PORT		9445

struct network_control
{
	NET_TYPE			*	port;
};


NETW_CTL *network_config_defaults( void );


#endif
