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
* selfstats.h - defines self reporting functions                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef CARBON_COPY_SELFSTATS_H
#define CARBON_COPY_SELFSTATS_H



#define DEFAULT_SELF_PREFIX			"self.carbon_copy."
#define DEFAULT_SELF_INTERVAL		10


enum self_timestamp_type
{
	SELF_TSTYPE_SEC = 0,
	SELF_TSTYPE_MSEC,
	SELF_TSTYPE_USEC,
	SELF_TSTYPE_NSEC,
	SELF_TSTYPE_NONE,
	SELF_TSTYPE_MAX
};

extern const char *self_ts_types[SELF_TSTYPE_MAX];


struct selfstats_control
{
	HOST				*	host;
	BUF					*	buf;
	char				*	prefix;
	char				*	ts;
	int64_t					intv;
	int64_t					tsdiv;
	int						tstype;
	int						enabled;
	int						tslen;
	uint32_t				prlen;
};


loop_call_fn self_stats_pass;
throw_fn self_stats_loop;
void self_stats_init( void );

SST_CTL *self_stats_config_defaults( void );
conf_line_fn self_stats_config_line;

#endif

