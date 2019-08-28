/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
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
	"compat"
};


const MODEL *metric_get_model( char *name )
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

int metric_get_type( char *name )
{
	int i;

	for( i = 0; i < METRIC_TYPE_MAX; ++i )
		if( !strcasecmp( name, metric_type_names[i] ) )
			return i;

	err( "Metric type '%s' not found.", name );
	return -1;
}


