/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* file/file.h - exposed structures and functions for file handling        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef ARCHIVIST_FILE_H
#define ARCHIVIST_FILE_H


#define RKV_FL_MAX_BUCKETS		7
#define DEFAULT_BASE_PATH		"/opt/archivist/files"

#define FILE_EXTENSION			".rkv"
#define FILE_EXTENSION_LEN		4

#define FILE_DEFAULT_THREADS	5

#define FILE_MAX_OPEN_SEC		120


enum file_value_metrics
{
	FILE_VAL_METR_MEAN = 0,
	FILE_VAL_METR_COUNT,
	FILE_VAL_METR_SUM,
	FILE_VAL_METR_MIN,
	FILE_VAL_METR_MAX,
	FILE_VAL_METR_SPREAD,
	FILE_VAL_METR_MIDDLE,
	FILE_VAL_METR_END
};


struct file_query
{
	RKQR				*	next;

	// where we put the points
	PNT					*	points;

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
	int						count;
	int						bkt;

};



// 16b
struct file_bucket
{
	int64_t					period;
	int64_t					count;
};


struct file_data
{
	RKFL				*	next;

	// actual file path
	char				*	fpath;

	// mmap pointer
	void				*	map;

	// points into the map
	RKHDR				*	hdr;

	// point to the cylinders
	void				*	ptrs[RKV_FL_MAX_BUCKETS];

	// file path length
	int16_t					fplen;

	// dir part length
	int16_t					dlen;

	int16_t					flags;
};


struct file_control
{
	char				*	base_path;
	RKBKT					default_buckets[RKV_FL_MAX_BUCKETS];

	int						bplen;
	int						maxpath;
	int8_t					bktct;
	int8_t					conf_bkt;

	int8_t					wr_threads;
	int						ms_sync;

	int						max_open_sec;
};


store_callback file_write_one;
store_callback file_finish_one;
throw_fn file_writer;

void file_read( RKFL *r, RKQR *qry );
void file_update( RKFL *r, PNT *points, int count );

int file_open( RKFL *r );
void file_close( RKFL *r );
void file_shutdown( RKFL *r );

RKFL *file_create_handle( char *path );

int file_scan_dir( BUF *fbuf, BUF *pbuf, TEL *prt, int depth );

int file_init( void );
FILE_CTL *file_config_defaults( void );
conf_line_fn file_config_line;


#endif
