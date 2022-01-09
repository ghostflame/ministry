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


enum monitor_type_values
{
	MON_TYPE_HTTP = 0,
	MON_TYPE_TCP,
	MON_TYPE_FILE,
	MON_TYPE_MAX
};

enum monitor_result_values
{
	MON_RES_SUCCESS = 0,
	MON_RES_FAILED,
	MON_RES_NO_RESULT,
	MON_RES_MAX
};

enum monitor_error_types
{
	MON_ERR_NONE = 0,
	MON_ERR_CONNECT_TIMEOUT,
	MON_ERR_CONNECT_REFUSED,
	MON_ERR_REQUEST_FAILED,
	MON_ERR_REQUEST_DENIED,
	MON_ERR_INVALID_PATH,
	MON_ERR_MAX
};

extern const char *monitor_type_names[MON_TYPE_MAX];
extern const char *monitor_result_strings[MON_RES_MAX];



struct monitor_data
{
	MON					*	next;

	char				*	name;

	// http
	char				*	user;
	char				*	pass;

	// http, file
	char				*	path;

	// http, tcp
	uint16_t				port;

	int8_t					active;		// 0 no, 1 yes
	int8_t					type;

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



// off in const.c - lookups
int monitor_type_lookup( const char *name );
const char *monitor_type_name( int mtype );
const char *monitor_error_label( int etype );




int monitors_init( void );


MONCFG *monitors_config_defaults( void );
int monitors_config_line( AVP *av );


#endif
