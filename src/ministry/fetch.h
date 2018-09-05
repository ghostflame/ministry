/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* fetch.h - defines structures and functions controlling data fetch       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_FETCH_H
#define MINISTRY_FETCH_H


#define DEFAULT_FETCH_BUF_SZ		0x20000		// 128k


struct fetch_metrics_map
{
	METMP			*	next;
	char			*	attr;
	int					alen;
	int					order;
};


struct fetch_target
{
	FETCH			*	next;

	char			*	name;
	char			*	remote;
	char			*	path;

	CURLWH			*	ch;

	HOST			*	host;
	DTYPE			*	dtype;
	METMP			*	attmap;

	line_fn			*	parser;
	add_fn			*	handler;

	int64_t				period;		// msec config, converted to usec
	int64_t				offset;		// msec config, converted to usec
	int64_t				bufsz;

	int					metrics;	// is it a metrics type?
	int					attct;		// attribute map count

	uint16_t			port;

	struct sockaddr_in	dst;
};




struct fetch_control
{
	FETCH			*	targets;
	int					fcount;
};


curlw_cb fetch_ministry;
curlw_cb fetch_metrics;


throw_fn fetch_loop;

int fetch_init( void );

conf_line_fn fetch_config_line;
FTCH_CTL *fetch_config_defaults( void );


#endif