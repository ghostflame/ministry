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
* filter.h - filtering structures and function declarations               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef METRIC_FILTER_FILTER_H
#define METRIC_FILTER_FILTER_H

#define DEFAULT_FILTER_DIR				"/etc/ministry/filters.d"

#define LINE_SEPARATOR					'\n'
#define MAX_PATH_SZ						4096

enum filter_modes
{
	FILTER_MODE_ALL = 0,
	FILTER_MODE_ALLOW,
	FILTER_MODE_DROP,
	FILTER_MODE_MAX
};


struct filter_data
{
	FILT			*	next;
	int					mode;
	RGXL			*	matches;
};

// add on to the host structure
struct filter_host
{
	FILT			*	filters;
	BUF				*	path;
	HOST			*	host;
	int					best_mode;
	int					running;
	int					gen;
};


struct filter_config
{
	FCONF			*	next;	// used if we have to keep old configs
	IPLIST			*	ipl;
	FTREE			*	watch;
	int					active;
	int					filters;
};

struct filter_control
{
	char			*	filter_dir;
	FCONF			*	fconf;

	int					close_conn;
	int					generation;
	int64_t				flush_max;

	struct timespec		chg_sleep;
	int					chg_delay;
	int					chg_signal;

	pthread_mutex_t		genlock;
};



ftree_callback filter_on_change;

tcp_fn filter_host_setup;
tcp_fn filter_host_end;
buf_fn filter_parse_buf;

throw_fn filter_host_watcher;
throw_fn filter_order_reload;

int filter_init( void );

FLT_CTL *filter_config_defaults( void );
conf_line_fn filter_config_line;


#endif
