/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
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
int relay_config_line( AVP *av );


#endif



