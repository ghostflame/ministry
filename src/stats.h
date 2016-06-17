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


loop_call_fn thread_pass;

stats_fn stats_stats_pass;
stats_fn stats_adder_pass;
stats_fn stats_gauge_pass;


throw_fn stats_loop;


enum stats_types
{
	STATS_TYPE_STATS = 0,
	STATS_TYPE_ADDER,
	STATS_TYPE_GAUGE,
	STATS_TYPE_SELF,
	STATS_TYPE_MAX
};


enum stats_ts_format
{
	STATS_TSF_SEC = 0,
	STATS_TSF_TVAL,
	STATS_TSF_TSPEC,
	STATS_TSF_MSEC,
	STATS_TSF_USEC,
	STATS_TSF_NSEC,
	STATS_TSF_MAX,
};



#define DEFAULT_STATS_THREADS		6
#define DEFAULT_ADDER_THREADS		2
#define DEFAULT_GAUGE_THREADS		2

#define DEFAULT_STATS_MSEC			10000

#define DEFAULT_STATS_PREFIX		"stats.timers"
#define DEFAULT_ADDER_PREFIX		""
#define DEFAULT_GAUGE_PREFIX		""


#define TSBUF_SZ					32


struct stat_thread_ctl
{
	ST_THR			*	next;
	ST_CFG			*	conf;
	char			*	wkrstr;
	int					id;
	int					max;
	pthread_mutex_t		lock;

	// workspace
	float			*	wkbuf;
	float			*	wkspc;
	int					wkspcsz;

	// output
	char			*	prefix;
	char			*	tsbuf;
	IOBUF			*	bp;
	uint16_t			tsbufsz;
	uint16_t			prlen;

	// current timestamp
	int64_t				tval;

	// counters
	int64_t				paths;
	int64_t				points;

	// timings
	struct timespec		now;
	// these are taken at the time of *starting* to do something
	struct timespec		steal;
	struct timespec		wait;
	struct timespec		stats;
	// and this is taken when finished
	struct timespec		done;
};



struct stat_config
{
	ST_THR			*	ctls;
	char			*	prefix;
	const char		*	name;
	int					type;
	int					dtype;
	int					threads;
	int					enable;
	int					prlen;
	int					period;		// msec config, converted to usec
	int					offset;		// msec config, converted to usec
	stats_fn		*	statfn;

	// and the data
	DHASH			**	data;
	int					hsize;
	int					dcurr;
	int					gc_count;
	uint32_t			did;
};


struct stat_threshold
{
	ST_THOLD		*	next;
	int					val;
	int					max;
	char			*	label;
};


struct stats_control
{
	ST_CFG			*	stats;
	ST_CFG			*	adder;
	ST_CFG			*	gauge;
	ST_CFG			*	self;

	int					ts_fmt;
	tsf_fn			*	ts_fp;

	ST_THOLD		*	thresholds;
};


void bprintf( ST_THR *t, char *fmt, ... );

void stats_start( ST_CFG *cf );
void stats_init( void );

STAT_CTL *stats_config_defaults( void );
int stats_config_line( AVP *av );


#endif
