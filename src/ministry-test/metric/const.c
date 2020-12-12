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
* metric/const.c - metrics config constants and lookup functions          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



const MODEL metric_model_types[METRIC_MODEL_MAX] =
{
	{
		.name    = "track_mean",
		.updater = &metric_update_track_mean,
		.model   = METRIC_MODEL_TRACK_MEAN,
	},
	{
		.name    = "track_mean_pos",
		.updater = &metric_update_track_mean_pos,
		.model   = METRIC_MODEL_TRACK_MEAN_POS,
	},
	{
		.name    = "floor_up",
		.updater = &metric_update_floor_up,
		.model   = METRIC_MODEL_FLOOR_UP,
	},
	{
		.name    = "sometimes_track",
		.updater = &metric_update_sometimes_track,
		.model   = METRIC_MODEL_SMTS_TRACK,
	}
};


const char *metric_type_names[METRIC_TYPE_MAX] =
{
	"adder",
	"stats",
	"gauge",
	"histo",
	"compat"
};


const MODEL *metric_get_model( const char *name )
{
	const MODEL *m;
	int i;

	for( i = 0; i < METRIC_MODEL_MAX; ++i )
	{
		m = metric_model_types + i;
		if( !strcasecmp( name, m->name ) )
			return m;
	}

	err( "Metric model '%s' not found.", name );
	return NULL;
}

int metric_get_type( const char *name )
{
	int i;

	i = str_search( name, metric_type_names, METRIC_TYPE_MAX );

	if( i < 0 )
		err( "Metric type '%s' not found.", name );

	return i;
}


