/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* filter.h - filtering structures and function declarations               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef METRIC_FILTER_FILTER_H
#define METRIC_FILTER_FILTER_H


#define DEFAULT_FILTER_DIR				"/etc/ministry/metric-filters.d"

enum filter_modes
{
	FILTER_MODE_ALL = 0,
	FILTER_MODE_ALLOW,
	FILTER_MODE_DROP,
	FILTER_MODE_MAX
};


struct filter_data
{
	HFILT			*	next;
	int					mode;
	RGXL			*	matches;
};

struct filter_list
{
	FILTL			*	next;
	HFILT			*	filter;
};

// add on to the host structure
struct filter_host
{
	FILTL			*	filters;
	int					best_mode;
};



struct filter_file
{
	FFILE			*	next;
	char			*	fname;
	int64_t				mtime;
	JSON			*	jo;
};

struct filter_config
{
	FCONF			*	next;	// used if we have to keep old configs
	IPLIST			*	ipl;
	FFILE			*	files;
	int					count;
};

struct filter_control
{
	char			*	filter_dir;
	FCONF			*	fconf;

	int					close_conn;
	int					generation;
	int					fldlen;

	pthread_mutex_t		genlock;
};

tcp_fn filter_host_setup;
tcp_fn filter_host_end;

throw_fn filter_watcher;

int filter_init( void );

conf_line_fn filter_config_line;
FLT_CTL *filter_config_defaults( void );


#endif
