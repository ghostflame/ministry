/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* filter.h - filtering structures and function declarations               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef METRIC_FILTER_FILTER_H
#define METRIC_FILTER_FILTER_H


struct filter_data
{
	HFILT			*	next;
	int					mode;
	RGXL			*	matches;
};


struct filter_config
{

};

struct filter_control
{



};


conf_line_fn filter_config_line;
FLT_CTL *filter_config_defaults( void );


#endif
