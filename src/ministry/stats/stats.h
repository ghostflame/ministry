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


json_callback stats_self_stats_cb_stats;
loop_call_fn stats_thread_pass;
throw_fn stats_loop;


#define STATS_HISTO_MAX			10
#define STATS_THRESH_MAX		20


enum stats_types
{
	STATS_TYPE_STATS = 0,
	STATS_TYPE_ADDER,
	STATS_TYPE_GAUGE,
	STATS_TYPE_HISTO,
	STATS_TYPE_SELF,
	STATS_TYPE_MAX
};



struct stat_predict_conf
{
	RGXL			*	rgx;
	pred_fn			*	fp;
	uint16_t			vsize;
	uint16_t			pmax;
	int8_t				enabled;
};


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
	int64_t				total;
	int64_t				highest;
	int64_t				predict;

	PMET			*	pm_pts;
	PMET			*	pm_high;
	PMET			*	pm_pct;
	PMET			*	pm_tot;

	// timings
	struct timespec		now;
	// these are taken at the time of *starting* to do something
	struct timespec		steal;
	struct timespec		wait;
	struct timespec		stats;
	// and this is taken when finished
	struct timespec		done;

	// percent of window used
	double				percent;
};


struct stat_moments
{
	RGXL			*	rgx;
	int64_t				min_pts;
	int					enabled;
};



struct stat_hist_conf
{
	ST_HIST			*	next;
	RGXL			*	rgx;
	char			*	name;
	double			*	bounds;

	int32_t				matches;

	int8_t				bcount;
	int8_t				brange; // == count - 1
	int8_t				enabled;
	int8_t				is_default;
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
	int64_t				period;		// msec config, converted to usec
	int64_t				offset;		// msec config, converted to usec
	stats_fn		*	statfn;

	// and the data
	DHASH			**	data;
	uint64_t			hsize;
	int					dcurr;
	LLCT				creates;
	LLCT				gc_count;
	uint32_t			did;
};


struct stats_metrics
{
	PMETS			*	source;
	PMETM			*	pts_total;
	PMETM			*	pts_count;
	PMETM			*	pts_high;
	PMETM			*	pct_time;
};



struct stats_control
{
	ST_CFG			*	stats;
	ST_CFG			*	adder;
	ST_CFG			*	gauge;
	ST_CFG			*	histo;
	ST_CFG			*	self;

	ST_THOLD		*	thresholds;
	ST_MOM			*	mom;
	ST_MOM			*	mode;
	ST_PRED			*	pred;

	ST_HIST			*	histcf;
	ST_HIST			*	histdefl;

	ST_MET			*	metrics;

	// for new sorting
	int32_t				qsort_thresh;
	int32_t				histcf_count;

	char				tags_char;
	int					tags_enabled;
};

// target timestamp functions
tsf_fn stats_tsf_sec;
tsf_fn stats_tsf_msec;
tsf_fn stats_tsf_usec;
tsf_fn stats_tsf_nsec;
tsf_fn stats_tsf_dotmsec;
tsf_fn stats_tsf_dotusec;
tsf_fn stats_tsf_dotnsec;


void stats_start( void );
void stats_init( void );
void stats_stop( void );

STAT_CTL *stats_config_defaults( void );
conf_line_fn stats_config_line;


#endif
