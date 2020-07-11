/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* synth/synth.h - defines synthetics structures                           *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_SYNTH_H
#define MINISTRY_SYNTH_H


struct synth_control
{
	SYNTH			*	list;
	int					scount;
	int					wait_usec;

	int					tcount;
	int					tready;
	int					tproceed;

	pthread_cond_t		threads_ready;
	pthread_cond_t      threads_done;
};


loop_call_fn synth_pass;

throw_fn synth_loop;


void synth_init( void );

SYN_CTL *synth_config_defaults( void );
conf_line_fn synth_config_line;


#endif
