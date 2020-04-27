/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* fetch/fetch.h - defines structures and functions controlling data fetch *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_FETCH_H
#define MINISTRY_FETCH_H


#define DEFAULT_FETCH_BUF_SZ		0x20000		// 128k



struct fetch_target
{
	FETCH			*	next;

	char			*	name;
	char			*	remote;
	char			*	path;
	char			*	profile;

	CURLWH			*	ch;

	HOST			*	host;
	DTYPE			*	dtype;
	MDATA			*	metdata;

	line_fn			*	parser;
	add_fn			*	handler;

	int64_t				period;		// msec config, converted to usec
	int64_t				offset;		// msec config, converted to usec
	int64_t				revalidate;	// msec config, converted to usec - recheck DNS
	int64_t				bufsz;
	int64_t				tval;		// current time

	struct sockaddr_in	dst;

	int8_t				metrics;	// is it a metrics type?
	int8_t				ready;
	int8_t				enabled;

	uint16_t			port;

};




struct fetch_control
{
	FETCH			*	targets;
	int					fcount;
};



throw_fn fetch_loop;

int fetch_init( void );

FTCH_CTL *fetch_config_defaults( void );
conf_line_fn fetch_config_line;


#endif

