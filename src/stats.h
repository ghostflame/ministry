#ifndef MINISTRY_STATS_H
#define MINISTRY_STATS_H


loop_call_fn stats_stats_pass;
loop_call_fn stats_adder_pass;

throw_fn stats_loop;


#define DEFAULT_ADDER_THREADS		2
#define DEFAULT_STATS_THREADS		6

#define DEFAULT_STATS_MSEC			10000

#define DEFAULT_STATS_PREFIX		"stats.timers"
#define DEFAULT_ADDER_PREFIX		""

#define DEFAULT_TARGET_HOST			"127.0.0.1"
#define DEFAULT_TARGET_PORT			2003			// graphite


struct stat_thread_ctl
{
	ST_THR			*	next;
	NSOCK			*	target;
	ST_CFG			*	conf;
	int					id;
	int					max;
	int					link;		// are we connected?
};


struct stat_config
{
	ST_THR			*	ctls;
	char			*	prefix;
	char			*	type;
	int					threads;
	int					period;		// msec
	int					offset;		// msec
	loop_call_fn	*	loopfn;

	// and the data
	DHASH			**	data;
	int					dcount;
};



struct stats_control
{
	ST_CFG			*	stats;
	ST_CFG			*	adder;

	char			*	host;
	struct sockaddr_in	target;
	unsigned short		port;
};


void stats_start( ST_CFG *cf );
void stats_init( void );

STAT_CTL *stats_config_defaults( void );
int stats_config_line( AVP *av );


#endif
