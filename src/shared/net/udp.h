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
* udp.h - defines UDP handling functions                                  *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_NET_UDP_H
#define SHARED_NET_UDP_H

throw_fn udp_loop_flat;
throw_fn udp_loop_checks;

int udp_listen( unsigned short port, uint32_t ip );

iplist_data_fn udp_add_phost;

#endif
