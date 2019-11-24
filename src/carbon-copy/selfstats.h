/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
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

