/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* app.h - startup/shutdown functions                                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#ifndef SHARED_APP_H
#define SHARED_APP_H

int set_signals( void );
int set_limits( void );

int app_init( char *name, char *cfgdir );
int app_start( int writePid );
void app_ready( void );
void app_finish( int exval );


#endif
