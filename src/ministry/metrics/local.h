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
* metrics/local.h - structures and functions for prometheus-style metrics *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_METRICS_LOCAL_H
#define MINISTRY_METRICS_LOCAL_H

#define METR_BUF_SZ			4096
#define LINE_SEP_CHAR		'\n'

#include "ministry.h"


enum metric_type_ids
{
	METR_TYPE_UNTYPED=0,
	METR_TYPE_COUNTER,
	METR_TYPE_SUMMARY,
	METR_TYPE_HISTOGRAM,
	METR_TYPE_GAUGE,
	METR_TYPE_MAX
};


// METTY
struct metrics_type
{
	char			*	name;
	int					nlen;
	int					type;
	int					dtype;
};


// METAT
struct metrics_attr
{
	METAT			*	next;
	char			*	name;
	int					len;
	int					order;
};



// one attributes list
// METAL
struct metrics_attr_list
{
	METAL			*	next;
	METAT			*	ats;
	char			*	name;

	int32_t				atct;
	int16_t				nlen;
	int16_t				refs;
};


/*
 * A metrics profile links incoming metrics to attribute maps
 *
 * It uses the normal regex lists, or direct entries
 *
 * You can pass the entire string in, not just the metric name,
 * depending on which function you call.  This lets you catch
 * interesting labels.
 */

// METMP
struct metrics_attr_map
{
	METMP			*	next;

	RGXL			*	rlist;

	char			*	lname;
	METAL			*	list;

	int					id;
	int					last;
	int					whole;
	int					enable;
};


// METPR
struct metrics_profile
{
	METPR			*	next;

	METMP			*	maps;
	SSTR			*	paths;

	char			*	default_att;
	METAL			*	default_attrs;

	char			*	name;
	int					nlen;
	int					mapct;
	int					_idctr;
	int					is_default;
};

extern const METTY metrics_types_defns[];
extern MET_CTL *_met;


// helpers.c
int metrics_cmp_attrs( const void *ap1, const void *ap2 );
int metrics_cmp_maps( const void *ap1, const void *ap2 );
void metrics_sort_attrs( METAL *a );
METPR *metrics_find_profile( const char *name );
METRY *metrics_find_entry( MDATA *m, const char *str, int len );


// metrics.c
void metrics_parse_buf( FETCH *f, IOBUF *b );
void metrics_add_entry( FETCH *f, METRY *parent );


int metrics_add_attr( METAL *m, char *str, int len );

#endif
