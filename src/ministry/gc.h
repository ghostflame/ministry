/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* gc.h - declares some gc routines                                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_GC_H
#define MINISTRY_GC_H

#define DEFAULT_GC_THRESH           8640        // 1 day @ 10s
#define DEFAULT_GC_GG_THRESH        25920       // 3 days @ 10s


struct gc_control
{
	int64_t					enabled;
	int64_t					thresh;
	int64_t					gg_thresh;
};


loop_call_fn gc_pass;
throw_fn gc_loop;
conf_line_fn gc_config_line;

GC_CTL *gc_config_defaults( void );

#endif
