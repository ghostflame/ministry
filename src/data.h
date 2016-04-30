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

// rounds the structure to 4k
#define PTLIST_SIZE				1021

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
	int					type;
	char			*	name;
	line_fn			*	lf;
	line_fn			*	pf;
	add_fn			*	af;
	uint16_t			port;
	char			*	sock;
};

extern const struct data_type_params data_type_defns[];


struct points_list
{
	PTLIST			*	next;
	uint32_t			count;
	float				vals[PTLIST_SIZE];
};

struct path_sum
{
	double				total;
	uint64_t			count;
};


union data_hash_vals
{
	PTLIST			*	points;
	PTSUM				sum;
};


struct data_hash_entry
{
	DHASH			*	next;
	char			*	path;

	DVAL				in;
	DVAL				proc;

	uint16_t			len;
	uint16_t			sz;
	uint32_t			id;
	uint32_t			sum;
	uint16_t			type;
	int16_t				empty;
};


DHASH *data_locate( char *path, int len, int type );

add_fn data_point_stats;
add_fn data_point_adder;
add_fn data_point_gauge;

void data_point_adder( char *path, int len, char *dat );

line_fn data_line_ministry;
line_fn data_line_compat;
line_fn data_line_min_prefix;
line_fn data_line_com_prefix;

void data_start( NET_TYPE *nt );

#endif
