#ifndef MINISTRY_LOOP_H
#define MINISTRY_LOOP_H

#define DEFAULT_TICK_USEC		10000



void loop_mark_start( char *tag );
void loop_mark_done( char *tag );

void loop_end( char *reason );
void loop_kill( int sig );

void loop_control( char *name, loop_call_fn *fp, void *arg, int usec, int sync, int offset );

void loop_start( void );

#endif
