/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* file/local.h                                                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef ARCHIVIST_FILE_LOCAL_H
#define ARCHIVIST_FILE_LOCAL_H


#define RKV_MAGIC_STR				"RKIV"

#define RKV_OPEN_FLAGS				O_RDWR|O_NOATIME
#define RKV_CREATE_MODE				0644


#include "archivist.h"


// 16b
struct file_header_start
{
	char					magic[4];
	uint8_t					version;
	int8_t					buckets;
	uint16_t				hdr_sz;
	int64_t					filesz;
};

// 16b
//struct file_bucket


// 128b
struct file_header
{
	// +0
	RKSTT					start;

	// +16
	RKBKT					buckets[RKV_FL_MAX_BUCKETS];
};


struct file_agg_entry
{
	int64_t					ts;
	int32_t					count;
	float					sum;
	float					min;
	float					max;
};


extern FILE_CTL *_file;


#endif
