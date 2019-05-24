/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* selfstats.h - defines self reporting functions                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_SELFSTATS_H
#define MINISTRY_SELFSTATS_H


stats_fn self_stats_pass;
http_callback self_stats_cb_stats;
pmet_fn self_stats_cb_metrics;

#define DEFAULT_SELF_PREFIX			"stats.ministry"


#endif
