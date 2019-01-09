/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* metrics.h - structures and functions for prometheus-style metrics       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_METRICS_H
#define MINISTRY_METRICS_H

#define METR_BUF_SZ			4096
#define METR_HASH_SZ		307

enum metric_type_ids
{
	METR_TYPE_UNTYPED=0,
	METR_TYPE_COUNTER,
	METR_TYPE_SUMMARY,
	METR_TYPE_HISTOGRAM,
	METR_TYPE_GAUGE,
	METR_TYPE_MAX
};


struct metrics_type
{
	char			*	name;
	int					nlen;
	int					type;
	int					dtype;
};


struct metrics_attr
{
	METAT			*	next;
	char			*	name;
	int					len;
	int					order;
};

struct metrics_entry
{
	METRY			*	next;
	METRY			*	parent;
	char			*	metric;
	METTY			*	mtype;
	DTYPE			*	dtype;
	add_fn			*	afp;
	int32_t				len;
	int32_t				sz;
};


// one attributes list
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
};



struct metrics_data
{
	METRY			**	entries;
	WORDS			*	wds;
	char			*	buf;
	char			**	aps;
	char			*	attrs;
	int16_t			*	apl;

	uint64_t			hsz;

	int64_t				lines;
	int64_t				broken;
	int64_t				unknown;

	int32_t				attct;
	int32_t				entct;
};


struct metrics_control
{
	METAL			*	alists;
	METPR			*	profiles;

	int32_t				alist_ct;
	int32_t				prof_ct;
};




curlw_cb metrics_fetch_cb;

void metrics_parse_buf( FETCH *f, IOBUF *b );
void metrics_add_entry( FETCH *f, METRY *parent );

void metrics_init_data( MDATA *m );
int metrics_add_attr( MDATA *m, char *str, int len );

int metrics_init( void );

conf_line_fn metrics_config_line;
MET_CTL *metrics_config_defaults( void );

#endif
