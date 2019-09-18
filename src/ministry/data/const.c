/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* data/const.c - constants for data handling                              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"


DTYPE data_type_defns[DATA_TYPE_MAX] =
{
	{
		.type = DATA_TYPE_STATS,
		.name = "stats",
		.lf   = &data_line_ministry,
		.pf   = &data_line_min_prefix,
		.af   = &data_point_stats,
		.uf   = &data_update_stats,
		.bp   = &data_parse_buf,
		.tokn = TOKEN_TYPE_STATS,
		.port = DEFAULT_STATS_PORT,
		.thrd = TCP_THRD_DSTATS,
		.styl = TCP_STYLE_THRD,
		.sock = "ministry stats socket",
		.nt   = NULL
	},
	{
		.type = DATA_TYPE_ADDER,
		.name = "adder",
		.lf   = &data_line_ministry,
		.pf   = &data_line_min_prefix,
		.af   = &data_point_adder,
		.uf   = &data_update_adder,
		.bp   = &data_parse_buf,
		.tokn = TOKEN_TYPE_ADDER,
		.port = DEFAULT_ADDER_PORT,
		.thrd = TCP_THRD_DADDER,
		.styl = TCP_STYLE_EPOLL,
		.sock = "ministry adder socket",
		.nt   = NULL
	},
	{
		.type = DATA_TYPE_GAUGE,
		.name = "gauge",
		.lf   = &data_line_ministry,
		.pf   = &data_line_min_prefix,
		.af   = &data_point_gauge,
		.uf   = &data_update_gauge,
		.bp   = &data_parse_buf,
		.tokn = TOKEN_TYPE_GAUGE,
		.port = DEFAULT_GAUGE_PORT,
		.thrd = TCP_THRD_DGAUGE,
		.styl = TCP_STYLE_EPOLL,
		.sock = "ministry gauge socket",
		.nt   = NULL
	},
	{
		.type = DATA_TYPE_HISTO,
		.name = "histo",
		.lf   = &data_line_ministry,
		.pf   = &data_line_min_prefix,
		.af   = &data_point_histo,
		.uf   = &data_update_histo,
		.bp   = &data_parse_buf,
		.tokn = TOKEN_TYPE_HISTO,
		.port = DEFAULT_HISTO_PORT,
		.thrd = TCP_THRD_DHISTO,
		.styl = TCP_STYLE_THRD,
		.sock = "ministry histogram socket",
		.nt   = NULL
	},
	{
		.type = DATA_TYPE_COMPAT,
		.name = "compat",
		.lf   = &data_line_compat,
		.pf   = &data_line_com_prefix,
		.af   = NULL,
		.bp   = &data_parse_buf,
		.tokn = 0,
		.port = DEFAULT_COMPAT_PORT,
		.thrd = TCP_THRD_DCOMPAT,
		.styl = TCP_STYLE_THRD,
		.sock = "statsd compat socket",
		.nt   = NULL
	},
};


