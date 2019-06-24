/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* pmet/utils.c - utility functions for prometheus metrics                 *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


double pmet_get_uptime( int64_t msec, void *arg, double *val )
{
	double up;

	up = get_uptime( );

	if( val )
		*val = up;

	return up;
}

double pmet_get_memory( int64_t msec, void *arg, double *val )
{
	double mem;

	mem = (double) ( 1024 * mem_curr_kb( ) );

	if( val )
		*val = mem;

	return mem;
}
