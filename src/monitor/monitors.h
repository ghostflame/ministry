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
* monitors.h - defines main monitor config                                *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef MONITOR_MONITORS_H
#define MONITOR_MONITORS_H

#define MON_DEFAULT_HTTP_PORT		9084
#define MON_DEFAULT_HTTPS_PORT		9447

#define DEFAULT_MONITORS_DIR		"/etc/ministry/monitors.d"
#define DEFAULT_INTERVAL_MSEC		10000

#define DEFAULT_MAX_MONITORS		512



struct monitor_data
{
	int32_t					intv_msec;


	int64_t					intv;
};



struct monitors_config
{
	char				*	mondir;
	int32_t					intv_msec;

	int						mon_ctr;
	int						max_ctr;


	int64_t					intv;
};


int monitors_init( void );


MONCFG *monitors_config_defaults( void );
int monitors_config_line( AVP *av );


#endif
