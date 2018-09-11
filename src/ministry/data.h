/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* data.h - defines data structures and format                             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#ifndef MINISTRY_DATA_H
#define MINISTRY_DATA_H

// rounds the structure to 16k with a spare space
#define PTLIST_SIZE				2045

#define LINE_SEPARATOR			'\n'
#define FIELD_SEPARATOR			' '

enum data_conn_type
{
	DATA_TYPE_STATS = 0,
	DATA_TYPE_ADDER,
	DATA_TYPE_GAUGE,
	DATA_TYPE_COMPAT,
	DATA_TYPE_MAX
};

struct data_type_params
{
	uint8_t				type;
	char			*	name;
	line_fn			*	lf;
	line_fn			*	pf;
	add_fn			*	af;
	int16_t				tokn;
	uint16_t			port;
	int64_t				thrd;
	char			*	sock;
	NET_TYPE		*	nt;
};

extern DTYPE data_type_defns[];


#define dp_set( _dp, t, v )			_dp.ts = t; _dp.val = v
#define dp_get_t( _dp )				_dp.ts
#define dp_get_v( _dp )				_dp.val

#define dpp_set( __dp, t, v )		__dp->ts = t; __dp->val = v
#define dpp_get_t( __dp )			__dp->ts
#define dpp_get_v( __dp )			__dp->val

struct data_point
{
	double				ts;
	double				val;
};


struct points_list
{
	PTLIST			*	next;
	int64_t				count;
	double				vals[PTLIST_SIZE];
	double				sentinel;
};


struct data_hash_vals
{
	PTLIST			*	points;
	double				total;
	uint64_t			count;
};


struct data_hash_entry
{
	DHASH			*	next;
	char			*	path;

	// in.points is pre-allocated by the stats pass
	// we cannot assume in.points non-null means we
	// have data
	DVAL				in;
	DVAL				proc;

	// predictor structure, present or absent
	PRED			*	predict;

	dhash_lock_t	*	lock;

	uint16_t			len;
	uint16_t			sz;
	uint32_t			id;

	uint64_t			sum;

	uint8_t				valid;
	uint8_t				do_pass;
	uint8_t				type;
	uint8_t				mom_check;
	int32_t				empty;
};


uint64_t data_path_hash_wrap( char *path, int len );

DHASH *data_locate( char *path, int len, int type );

add_fn data_point_stats;
add_fn data_point_adder;
add_fn data_point_gauge;

line_fn data_line_ministry;
line_fn data_line_token;
line_fn data_line_compat;
line_fn data_line_min_prefix;
line_fn data_line_com_prefix;

void data_parse_buf( HOST *h, IOBUF *b );

void data_start( NET_TYPE *nt );

#endif
