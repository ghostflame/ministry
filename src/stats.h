/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* stats.h - defines stats structures and routines                         *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_STATS_H
#define MINISTRY_STATS_H


loop_call_fn stats_stats_pass;
loop_call_fn stats_adder_pass;
loop_call_fn stats_gauge_pass;
loop_call_fn stats_self_pass;


throw_fn stats_loop;


enum stats_types
{
	STATS_TYPE_STATS = 0,
	STATS_TYPE_ADDER,
	STATS_TYPE_GAUGE,
	STATS_TYPE_SELF,
	STATS_TYPE_MAX
};



#define DEFAULT_STATS_THREADS		6
#define DEFAULT_ADDER_THREADS		2
#define DEFAULT_GAUGE_THREADS		2

#define DEFAULT_STATS_MSEC			10000

#define DEFAULT_STATS_PREFIX		"stats.timers"
#define DEFAULT_ADDER_PREFIX		""
#define DEFAULT_GAUGE_PREFIX		""
#define DEFAULT_SELF_PREFIX			"stats.ministry"



struct stat_thread_ctl
{
	ST_THR			*	next;
	ST_CFG			*	conf;
	int					id;
	int					max;
	int					points;
	int					active;
	pthread_mutex_t		lock;

	// workspace
	float			*	wkbuf;
	float			*	wkspc;
	int					wkspcsz;
};


struct stat_config
{
	ST_THR			*	ctls;
	char			*	prefix;
	const char		*	name;
	int					type;
	int					threads;
	int					enable;
	int					period;		// msec
	int					offset;		// msec
	loop_call_fn	*	loopfn;

	// and the data
	DHASH			**	data;
	int					dcurr;
	int					gc_count;
	uint32_t			did;
};



struct stats_control
{
	ST_CFG			*	stats;
	ST_CFG			*	adder;
	ST_CFG			*	gauge;
	ST_CFG			*	self;

	int				*	thresholds;
	int					thr_count;
};


void stats_start( ST_CFG *cf );
void stats_init( void );

STAT_CTL *stats_config_defaults( void );
int stats_config_line( AVP *av );


#endif
