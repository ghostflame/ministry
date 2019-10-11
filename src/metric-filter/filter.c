/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* filter.c - filtering functions                                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "metric_filter.h"



FLT_CTL *filter_config_defaults( void )
{
	FLT_CTL *f = (FLT_CTL *) allocz( sizeof( FLT_CTL ) );



	return f;
}

int filter_config_line( AVP *av )
{



}
