/**************************************************************************
* Copyright 2015 John Denholm                                             *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
*                                                                         *
*                                                                         *
* query/defs.c - query data function definitions                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

QRFN query_function_defns[11] =
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
	},
	{
		.name  = "summarize",
		.fn    = &query_xform_summarize,
		.argc  = 2,
		.argm  = 1,
		.types = { QARG_TYPE_INTEGER, QARG_TYPE_INTEGER, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},

	// combine series

	{
		.name  = "sum",
		.fn    = &query_combine_sum,
		.argc  = 0,
		.argm  = 0,
		.types = { QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},
	{
		.name  = "average",
		.fn    = &query_combine_average,		
		.argc  = 0,
		.argm  = 0,
		.types = { QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE, QARG_TYPE_NONE }
	},
	{
		.name  = "presence",
		.fn    = &query_combine_presence,
		.argc  = 2,
		.argm  = 0,
		.types = { QARG_TYPE_INTEGER, QARG_TYPE_INTEGER, QARG_TYPE_NONE, QARG_TYPE_NONE }
	}
};





int query_parse( QRY *q )
{
	debug( "Query parse called." );
	return 0;
}
























