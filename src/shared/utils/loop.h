/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* loop.h - defines run control / loop control fns                         *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#ifndef SHARED_LOOP_H
#define SHARED_LOOP_H

#define DEFAULT_TICK_MSEC		5
#define MAX_LOOP_NSEC			5000000000

#define LOOP_TRIM				0x01
#define LOOP_SYNC				0x02
#define LOOP_DEBUG				0x04


void loop_mark_start( const char *tag );
void loop_mark_done( const char *tag, int64_t skips, int64_t fires );

void loop_end( const char *reason );
void loop_kill( int sig );

throw_fn loop_timer;

void loop_control( const char *name, loop_call_fn *fp, void *arg, int64_t usec, int flags, int64_t offset );


#endif
