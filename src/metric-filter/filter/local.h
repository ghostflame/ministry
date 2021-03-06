/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* filter/local.h - filtering structs not exposed                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef METRIC_FILTER_FILTER_LOCAL_H
#define METRIC_FILTER_FILTER_LOCAL_H


#define FILTER_DEFAULT_FLUSH		1000		// msec
#define FILTER_DEFAULT_DELAY		10000		// msec



#include "metric_filter.h"


#define lock_filters( )				pthread_mutex_lock(   &(ctl->filt->genlock) )
#define unlock_filters( )			pthread_mutex_unlock( &(ctl->filt->genlock) )




extern const char *filter_mode_name[];
extern FLT_CTL *_filt;


int filter_load( void );
void filter_unlock( void );



#endif
