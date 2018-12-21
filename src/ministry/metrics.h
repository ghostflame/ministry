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


struct metrics_map
{
	METMP			*	next;
	char			*	attr;
	int					alen;
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


struct metrics_data
{
	METRY			**	entries;
	METMP			*	attrs;
	WORDS			*	wds;
	char			*	buf;
	char			**	aps;
	int16_t			*	apl;

	uint64_t			hsz;

	int64_t				lines;
	int64_t				broken;
	int64_t				unknown;

	int32_t				attct;
	int32_t				entct;
};


curlw_cb metrics_fetch_cb;

void metrics_parse_buf( FETCH *f, IOBUF *b );

void metrics_add_entry( FETCH *f, METRY *parent );

void metrics_init_data( MDATA *m );
int metrics_add_attr( MDATA *m, char *str, int len );

#endif
