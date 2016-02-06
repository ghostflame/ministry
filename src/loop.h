#ifndef MINISTRY_LOOP_H
#define MINISTRY_LOOP_H

#define DEFAULT_TICK_MSEC		20



void loop_mark_start( const char *tag );
void loop_mark_done( const char *tag );

void loop_end( char *reason );
void loop_kill( int sig );

void loop_control( const char *name, loop_call_fn *fp, void *arg, int usec, int sync, int offset );

void loop_start( void );

#endif
