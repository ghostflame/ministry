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
* ha.h - defines high-availability structures and functions               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#ifndef SHARED_HA_H
#define SHARED_HA_H



/*
struct ha_partner
{
	HAPT			*	next;
	char			*	host;
	char			*	name;	// host:port

	char			*	check_url;
	char			*	ctl_url;
	CURLWH			*	ch;
	IOBUF			*	buf;

	uint16_t			port;
	int16_t				nlen;

	int64_t				tmout;
	int64_t				tmout_orig;
	int					use_tls;
	int					is_master;
	int					is_self;
};

*/


struct ha_control
{
	HAPT			*	partners;
	HAPT			*	self;
	int					pcount;

	pthread_mutex_t		lock;

	char			*	hostname;

	int64_t				period;		// msec
	int64_t				update; 	// msec

	int					priority;
	int					elector;	// style of election

	int					is_master;
	int					enabled;
};


curlw_cb ha_watcher_cb;
curlw_jcb ha_watcher_jcb;

loop_call_fn ha_watcher_pass;
throw_fn ha_watcher;

loop_call_fn ha_controller_pass;
throw_fn ha_controller;

http_callback ha_get_cluster;
http_callback ha_ctl_cluster;

int ha_init( void );
int ha_start( void );
void ha_shutdown( void );

HA_CTL *ha_config_defaults( void );
conf_line_fn ha_config_line;


#endif
