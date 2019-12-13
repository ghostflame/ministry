/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* filter/init.c - filtering startup functions                             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


void filter_stop( void )
{
	return;
}


int filter_init( void )
{
	int ret;

	pthread_mutex_init( &(ctl->filt->genlock), NULL );

	// convert msec to nsec
	ctl->filt->chg_delay *= MILLION;
	ctl->filt->chg_sleep.tv_sec  = ctl->filt->chg_delay / BILLION;
	ctl->filt->chg_sleep.tv_nsec = ctl->filt->chg_delay % BILLION;

	lock_filters( );
	ret = filter_load( );
	unlock_filters( );

	return ret;
}
