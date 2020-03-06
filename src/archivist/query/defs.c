/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* query/defs.c - query data function definitions                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

QRFN query_function_defns[7] =
{
	// transform one series
	{
		.name  = "scale",
		.fn    = &query_xform_scale,
		.argc  = 1,
		.argm  = 1,
		.types = { QARG_TYPE_DOUBLE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},
	{
		.name  = "offset",
		.fn    = &query_xform_scale,
		.argm  = 1,
		.argc  = 1,
		.types = { QARG_TYPE_DOUBLE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},
	{
		.name  = "abs",
		.fn    = &query_xform_scale,
		.argc  = 0,
		.argm  = 0,
		.types = { QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},
	{
		.name  = "log",
		.fn    = &query_xform_scale,
		.argc  = 1,
		.argm  = 0,
		.types = { QARG_TYPE_INTEGER, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},
	{
		.name  = "derivative",
		.fn    = &query_xform_scale,
		.argc  = 0,
		.argm  = 0,
		.types = { QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},
	{
		.name  = "non_negative_derivative",
		.fn    = &query_xform_scale,
		.argc  = 0,
		.argm  = 0,
		.types = { QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},
	{
		.name  = "integral",
		.fn    = &query_xform_scale,
		.argc  = 0,
		.argm  = 0,
		.types = { QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	}
};





int query_parse( QRY *q )
{
	return 0;
}
























