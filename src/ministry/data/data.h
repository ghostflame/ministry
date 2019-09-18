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
#define PTLIST_SIZE				2046

#define DHASH_CHECK_MOMENTS		0x01
#define DHASH_CHECK_MODE		0x02
#define DHASH_CHECK_PREDICT		0x04


enum data_conn_type
{
	DATA_TYPE_STATS = 0,
	DATA_TYPE_ADDER,
	DATA_TYPE_GAUGE,
	DATA_TYPE_HISTO,
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
	dupd_fn			*	uf;
	buf_fn			*	bp;
	int16_t				tokn;
	uint16_t			port;
	int64_t				thrd;
	int8_t				styl;
	char			*	sock;
	NET_TYPE		*	nt;
	ST_CFG			*	stc;
};

extern DTYPE data_type_defns[];


#define dp_set( _dp, t, v )			_dp.ts = t; _dp.val = v
#define dp_get_t( _dp )				_dp.ts
#define dp_get_v( _dp )				_dp.val

#define dpp_set( __dp, t, v )		__dp->ts = t; __dp->val = v
#define dpp_get_t( __dp )			__dp->ts
#define dpp_get_v( __dp )			__dp->val


#define dhash_do_moments( _d )		( _d->checks & DHASH_CHECK_MOMENTS )
#define dhash_do_mode( _d )			( _d->checks & DHASH_CHECK_MODE )
#define dhash_do_predict( _d )		( ( _d->checks & DHASH_CHECK_PREDICT ) && _d->predict )


struct data_point
{
	double				ts;
	double				val;
};


struct data_histogram	// size 16
{
	int64_t			*	counts;
	ST_HIST			*	conf;
};


struct points_list		// 8192
{
	PTLIST			*	next;
	int64_t				count;
	double				vals[PTLIST_SIZE];
};


struct data_hash_vals	// size 40
{
	PTLIST			*	points;
	DHIST				hist;
	double				total;
	int64_t				count;
};


struct data_hash_entry	// size 104
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
	uint8_t				checks;
	int32_t				empty;
};


uint64_t data_path_hash_wrap( char *path, int len );

DHASH *data_locate( char *path, int len, int type );
DHASH *data_get_dhash( char *path, int len, ST_CFG *c );


dupd_fn data_update_stats;
dupd_fn data_update_histo;
dupd_fn data_update_adder;
dupd_fn data_update_gauge;

add_fn data_point_stats;
add_fn data_point_adder;
add_fn data_point_gauge;
add_fn data_point_histo;

line_fn data_line_ministry;
line_fn data_line_compat;
line_fn data_line_min_prefix;
line_fn data_line_com_prefix;

curlw_cb  data_fetch_cb;
curlw_jcb data_fetch_jcb;

buf_fn data_parse_buf;

// handle json data
int data_parse_json( json_object *jo, DTYPE *dt );

void data_start( NET_TYPE *nt );

#endif
