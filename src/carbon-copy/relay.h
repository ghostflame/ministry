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


#define LINE_SEPARATOR				'\n'
#define RELAY_MAX_REGEXES			32
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

#define RTYPE_REGEX					1
#define RTYPE_HASH					2


struct relay_line
{
	char				*	path;
	char				*	rest;

	int						plen;
	int						rlen;
};


struct relay_data
{
	RDATA				*	next;

	HOST				*	host;
	int64_t					last;
	int						running;

	pthread_mutex_t			lock;

	HBUFS				*	hbufs;
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

	int						bcount;
};


relay_fn relay_regex;
relay_fn relay_hash;

line_fn relay_simple;
line_fn relay_prefix;

throw_fn relay_track_host;

tcp_fn relay_flush_host;
tcp_fn relay_buf_end;
tcp_fn relay_buf_set;


buf_fn relay_parse_buf;

int relay_resolve( void );

RLY_CTL *relay_config_defaults( void );
int relay_config_line( AVP *av );


#endif

