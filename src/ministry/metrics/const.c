/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* metrics/const.c - constants defining metric types                       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


// NOTE:  These *are* in METR_TYPE order, so array[type] should work
const METTY metrics_types_defns[METR_TYPE_MAX] =
{
	{
		.name  = "Untyped",
		.nlen  = 7,
		.type  = METR_TYPE_UNTYPED,
		.dtype = DATA_TYPE_ADDER,
	},
	{
		.name  = "Counter",
		.nlen  = 7,
		.type  = METR_TYPE_COUNTER,
		.dtype = DATA_TYPE_ADDER,
	},
	{
		.name  = "Summary",
		.nlen  = 7,
		.type  = METR_TYPE_SUMMARY,
		.dtype = DATA_TYPE_ADDER,
	},
	{
		.name  = "Histogram",
		.nlen  = 9,
		.type  = METR_TYPE_HISTOGRAM,
		.dtype = DATA_TYPE_ADDER,
	},
	{
		.name  = "Gauge",
		.nlen  = 5,
		.type  = METR_TYPE_GAUGE,
		.dtype = DATA_TYPE_GAUGE,
	}
};



