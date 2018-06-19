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

tsf_fn stats_tsf_sec;
tsf_fn stats_tsf_msec;
tsf_fn stats_tsf_usec;
tsf_fn stats_tsf_nsec;
tsf_fn stats_tsf_dotmsec;
tsf_fn stats_tsf_dotusec;
tsf_fn stats_tsf_dotnsec;


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

#define DEFAULT_MOM_MIN				30L

#define TSBUF_SZ					32
#define PREFIX_SZ					512
#define PATH_SZ						8192

#define DEFAULT_STATS_PREDICT		30
#define MAX_STATS_PREDICT			254
#define MIN_STATS_PREDICT			8


struct stat_thread_ctl
{
	ST_THR			*	next;
	ST_CFG			*	conf;
	char			*	wkrstr;
	uint64_t			id;
	int					max;
	pthread_mutex_t		lock;

	// workspace
	double			*	wkbuf1;		// first buffer
	double			*	wkbuf2;		// second buffer
	double			*	wkspc;		// source buffer
	DPT				*	predbuf;	// prediction buffer
	uint32_t		*	counters;
	int32_t				wkspcsz;

	// output
	BUF				*	prefix;
	BUF				*	path;
	BUF				**	ts;
	IOBUF			**	bp;

	// current timestamp
	int64_t				tval;

	// counters
	int64_t				active;
	int64_t				points;
	int64_t				highest;

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
	BUF				*	prefix;
	const char		*	name;
	int					type;
	int					dtype;
	int					threads;
	int					enable;
	int					period;		// msec config, converted to usec
	int					offset;		// msec config, converted to usec
	stats_fn		*	statfn;

	// and the data
	DHASH			**	data;
	uint64_t			hsize;
	int					dcurr;
	LLCT				gc_count;
	uint32_t			did;
};


struct stat_threshold
{
	ST_THOLD		*	next;
	int					val;
	int					max;
	char			*	label;
};


struct stat_moments
{
	RGXL			*	rgx;
	int64_t				min_pts;
	int					enabled;
};

struct stat_predict
{
	RGXL			*	rgx;
	uint8_t				vsize;
	uint8_t				pmax;
	int8_t				enabled;
};


struct stats_control
{
	ST_CFG			*	stats;
	ST_CFG			*	adder;
	ST_CFG			*	gauge;
	ST_CFG			*	self;

	ST_THOLD		*	thresholds;
	ST_MOM			*	mom;
	ST_PRED			*	pred;

	// for new sorting
	int32_t				qsort_thresh;
};


void bprintf( ST_THR *t, char *fmt, ... );

void stats_start( ST_CFG *cf );
void stats_init( void );

STAT_CTL *stats_config_defaults( void );
int stats_config_line( AVP *av );


#endif
