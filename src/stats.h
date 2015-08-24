#ifndef MINISTRY_STATS_H
#define MINISTRY_STATS_H


loop_call_fn stats_run_stats;
loop_call_fn stats_run_adder;

throw_fn stats_stats_pass;
throw_fn stats_adder_pass;

throw_fn stats_loop_stats;
throw_fn stats_loop_adder;


#define DEFAULT_ADDER_THREADS		2
#define DEFAULT_STATS_THREADS		6


struct stats_control
{
	int					stats_threads;
	int					adder_threads;
};

STAT_CTL *stats_config_defaults( void );
int stats_config_line( AVP *av );


#endif
