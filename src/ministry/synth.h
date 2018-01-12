/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* synth.h - defines synthetics structures                                 *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_SYNTH_H
#define MINISTRY_SYNTH_H

#define SYNTH_PART_MAX			32


// just used for parsing config lines
struct synth_fn_def
{
	char			*	names[3];
	synth_fn		*	fn;
	int					min_parts;
	int					max_parts;
	int					max_absent;
};


struct synth_data
{
	SYNTH			*	next;

	DHASH			*	dhash[SYNTH_PART_MAX];
	char			*	paths[SYNTH_PART_MAX];
	int					plens[SYNTH_PART_MAX];

	DHASH			*	target;
	char			*	target_path;

	double				factor;			// defaults to 1

	int					enable;
	int					parts;
	int					missing;

	int					absent;
	int					max_absent;

	SYNDEF			*	def;
};


struct synth_control
{
	SYNTH			*	list;
	int					scount;
	int					wait_usec;
};





synth_fn synth_sum;
synth_fn synth_diff;
synth_fn synth_div;
synth_fn synth_prod;
synth_fn synth_max;
synth_fn synth_min;
synth_fn synth_spread;
synth_fn synth_mean;
synth_fn synth_count;
synth_fn synth_active;

loop_call_fn synth_pass;

throw_fn synth_loop;


void synth_init( void );

SYN_CTL *synth_config_defaults( void );
int synth_config_line( AVP *av );


#endif
