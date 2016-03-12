/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* loop.h - defines run control / loop control fns                         *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#ifndef MINISTRY_LOOP_H
#define MINISTRY_LOOP_H

#define DEFAULT_TICK_MSEC		20
#define MAX_LOOP_USEC			3000000


void loop_mark_start( const char *tag );
void loop_mark_done( const char *tag );

void loop_end( char *reason );
void loop_kill( int sig );

void loop_control( const char *name, loop_call_fn *fp, void *arg, int usec, int sync, int offset );

void loop_start( void );

#endif
