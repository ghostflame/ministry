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

	char			*	url;
	char			*	name;
	HOST			*	h;
	DTYPE			*	dtype;
	METMP			*	attmap;

	int64_t				period;		// msec config, converted to usec
	int64_t				offset;		// msec config, converted to usec

	int					metrics;	// is it a metrics type?
	int					type;		// DATA_ type
	int					is_ssl;
};




struct fetch_control
{
	FETCH			*	targets;
	int					fcount;
};


throw_fn fetch_loop;

conf_line_fn fetch_config_line;
FTCH_CTL *fetch_config_defaults( void );


#endif
