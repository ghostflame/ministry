/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* data.h - defines data simulator structures                              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef MINISTRY_TEST_DATA_H
#define MINISTRY_TEST_DATA_H



struct data_metric
{
	METRIC				*	next;
	data_fn				*	fp;
	char				*	path;

	double					state1;
	double					state2;
	double					state3;
};


struct data_metric_group
{
	METRIC				*	list;
	BUF					*	prefix;
	TARGET				*	target;
	data_fn				*	fp;

	int64_t					mcount;
};

struct data_controller
{
	CTLR				*	next;
	char				*	name;

	MGRP				*	adder;
	MGRP				*	stats;
	MGRP				*	gauge;
};


#endif
