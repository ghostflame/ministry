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

// types we will need
typedef struct metrics_type			METTY;
typedef struct metrics_attr			METAT;
typedef struct metrics_attr_list	METAL;
typedef struct metrics_attr_map		METMP;
typedef struct metrics_profile		METPR;

// size of the metrics hash
#define METR_HASH_SZ				307

// METRY
struct metrics_entry
{
	METRY			*	next;
	METRY			*	parent;
	char			*	metric;
	const METTY		*	mtype;
	DTYPE			*	dtype;
	add_fn			*	afp;
	METAL			*	attrs;
	char			**  aps;
	int16_t			*	apl;
	int32_t				len;
	int32_t				sz;
};


// MDATA
struct metrics_data
{
	SSTR			*	entries;
	WORDS			*	wds;
	char			*	buf;
	char			*	profile_name;
	METPR			*	profile;

	uint64_t			hsz;

	int64_t				lines;
	int64_t				broken;
	int64_t				unknown;

	int32_t				attct;
	int32_t				entct;
};


// MET_CTL
struct metrics_control
{
	METAL			*	alists;
	METPR			*	profiles;

	int32_t				alist_ct;
	int32_t				prof_ct;
};



curlw_cb metrics_fetch_cb;

void metrics_init_data( MDATA *m );
int metrics_init( void );

conf_line_fn metrics_config_line;
MET_CTL *metrics_config_defaults( void );

#endif
