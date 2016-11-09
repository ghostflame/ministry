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

// rounds the structure to 8k with a spare space
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
	uint8_t				type;
	char			*	name;
	line_fn			*	lf;
	line_fn			*	pf;
	add_fn			*	af;
	int16_t				tokn;
	uint16_t			port;
	uint32_t			lock;
	char			*	sock;
};

extern const DTYPE data_type_defns[];


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

	DVAL				in;
	DVAL				proc;

	uint16_t			len;
	uint16_t			sz;
	uint32_t			id;
	uint32_t			sum;
	uint8_t				valid;
	uint8_t				do_pass;
	uint8_t				type;
	uint8_t				_pad;
	int32_t				empty;
};


DHASH *data_locate( char *path, int len, int type );

add_fn data_point_stats;
add_fn data_point_adder;
add_fn data_point_gauge;

void data_point_adder( char *path, int len, char *dat );

line_fn data_line_ministry;
line_fn data_line_token;
line_fn data_line_compat;
line_fn data_line_min_prefix;
line_fn data_line_com_prefix;

int data_parse_buf( HOST *h, char *buf, int len );
void data_start( NET_TYPE *nt );

#endif
