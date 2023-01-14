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



#ifndef CARBON_COPY_RELAY_H
#define CARBON_COPY_RELAY_H

#define DEFAULT_RELAY_PORT			3901
#define RELAY_MAX_TARGETS			32

#define lock_rdata( _r )			pthread_mutex_lock(   &(_r->lock) )
#define unlock_rdata( _r )			pthread_mutex_unlock( &(_r->lock) )



enum relay_rule_types
{
	RTYPE_UNKNOWN = 0,
	RTYPE_REGEX,
	RTYPE_HASH,
	RTYPE_MAX
};



struct host_buffers
{
	HBUFS				*	next;
	RELAY				*	rule;
	IOBUF				**	bufs;
	int						bcount;
};


struct relay_data
{
	RDATA				*	next;

	HOST				*	host;
	HBUFS				*	hbufs;

	pthread_mutex_t			lock;

	int64_t					last;
	int8_t					running;
	int8_t					linit;
	int8_t					shutdown;
};


struct relay_rule
{
	RELAY				*	next;
	TGTL				**	targets;
	regex_t				*	matches;
	char				**  rgxstr;
	uint8_t				*	invert;
	int64_t				*	mstats;
	char				*	name;
	char				*	target_str;
	hash_fn				*	hfp;
	relay_fn			*	rfp;

	int64_t					lines;

	int						type;
	int						last;
	int						tcount;
	int						mcount;
	int						drop;
};



struct relay_control
{
	RELAY				*	rules;
	int64_t					flush_nsec;
	NET_TYPE			*	net;

	int						bcount;
};


relay_fn relay_regex;
relay_fn relay_hash;

line_fn relay_simple;
line_fn relay_prefix;

tcp_fn relay_buf_end;
tcp_fn relay_buf_set;

buf_fn relay_parse_buf;

int relay_init( void );

RLY_CTL *relay_config_defaults( void );
conf_line_fn relay_config_line;


#endif



