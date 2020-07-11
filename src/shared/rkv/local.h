/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* rkv/local.h - local structures for archive files                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef SHARED_RKV_LOCAL_H
#define SHARED_RKV_LOCAL_H


#define RKV_MAGIC_STR				"RKIV"

#define RKV_OPEN_FLAGS				O_RDWR|O_NOATIME
#define RKV_CREATE_MODE				0644

#define RKV_DEFAULT_RET				"10s:2d;60s:30d"
#define RKV_DEFAULT_MAXPATH			512

#include "shared.h"


// 16b
struct rkv_header_start
{
	char					magic[4];
	uint8_t					version;
	int8_t					buckets;
	uint16_t				hdr_sz;
	int64_t					filesz;
};


// 128b
struct rkv_header
{
	// +0
	RKSTT					start;

	// +16
	RKBKT					buckets[RKV_FL_MAX_BUCKETS];
};



struct rkv_agg_entry
{
	int64_t					ts;
	int32_t					count;
	float					sum;
	float					min;
	float					max;
};



struct rkv_bucket_match
{
	RKBMT				*	next;
	char				*	name;
	RGXL				*	rgx;
	RKBKT					buckets[RKV_FL_MAX_BUCKETS];
	int						bct;
	int						is_default;
};


struct rkv_file_thread
{
	SSTR				*	hash;

	PMET				*	updates;
	PMET				*	pass_time;

	int64_t					id;

	int64_t					up_ops;
	double					up_time;
};




extern RKV_CTL *_rkv;

TEL *rkv_tree_insert_node( TEL *prt, const char *name );
int rkv_tree_insert_leaf( TEL *prt, const char *name, const char *path );

void rkv_writer( THRD *t );

int rkv_parse_buckets( RKBMT *m, const char *str, int len );

#endif
