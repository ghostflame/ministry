/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* rkv/rkv.h - exposed structures and functions for file handling          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_RKV_H
#define SHARED_RKV_H


#define RKV_FL_MAX_BUCKETS		7
#define DEFAULT_BASE_PATH		"/opt/archivist/files"

#define RKV_EXTENSION			".rkv"
#define RKV_EXTENSION_LEN		4

#define RKV_DEFAULT_THREADS	5

#define RKV_MAX_OPEN_SEC		120


#define PTS_MAX_SMALL			256		// 2k structure
#define PTS_MAX_MEDIUM			2048	// 16k structure




// PNT
struct data_point
{
	int64_t				ts;
	double				val;
};

// 24b + points
struct data_series
{
	PTL				*	next;
	PNT				*	points;
	int32_t				count;
	int32_t				sz;
};




enum rkv_value_metrics
{
	RKV_VAL_METR_MEAN = 0,
	RKV_VAL_METR_COUNT,
	RKV_VAL_METR_SUM,
	RKV_VAL_METR_MIN,
	RKV_VAL_METR_MAX,
	RKV_VAL_METR_SPREAD,
	RKV_VAL_METR_MIDDLE,
	RKV_VAL_METR_RANGE,
	RKV_VAL_METR_END
};


struct rkv_query
{
	RKQR				*	next;

	// which file?
	RKFL				*	fl;

	// which bucket
	RKBKT				*	b;

	// where we put the points
	PTL					*	data;

	// original requested timestamps
	int64_t					from;
	int64_t					to;

	// timestamps provided by bucket
	int64_t					first;
	int64_t					last;

	// offsets - array of 2 if needed
	int64_t					oa[2];
	int64_t					ob[2];

	int						metric;
	int						bkt;
};


struct rkv_entry
{
	int64_t					ts;
	double					val;
};


// 16b
struct rkv_bucket
{
	int64_t					period;
	int64_t					count;
};



struct rkv_data
{
	RKFL				*	next;

	// actual file path
	char				*	fpath;

	// metric path
	char				*	path;

	// pointer back to an owning struct
	void				*	ptr;

	// mmap pointer
	void				*	map;

	// points into the map
	RKHDR				*	hdr;

	// point to the cylinders
	void				*	ptrs[RKV_FL_MAX_BUCKETS];

	// file path length
	int16_t					fplen;

	// metric path len
	int16_t					plen;

	// dir part length
	int16_t					dlen;

	int16_t					flags;
};


struct rkv_control
{
	char				*	base_path;

	RKBMT				*	retentions;
	RKBMT				*	ret_default;

	int						bplen;
	int						maxpath;

	int						wr_threads;
	int						ms_sync;

	int						max_open_sec;
	int						rblocks;
};


store_callback rkv_write_one;
store_callback rkv_finish_one;
throw_fn rkv_writer;


int rkv_choose_metric( const char *name );
const char *rkv_report_metric( int mval );

void rkv_read( RKFL *r, RKQR *qry );
void rkv_update( RKFL *r, PNT *points, int count );

int rkv_open( RKFL *r );
void rkv_close( RKFL *r );
void rkv_shutdown( RKFL *r );

RKFL *rkv_create_handle( char *path, int plen );

int rkv_scan_dir( BUF *fbuf, BUF *pbuf, int depth, void *cbarg, rkv_dir_cb_fn *dfp, rkv_file_cb_fn *ffp );

int rkv_init( void );
RKV_CTL *rkv_config_defaults( void );
conf_line_fn rkv_config_line;


#endif

