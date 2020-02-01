/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* data.h - incoming data handling function prototypes                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef ARCHIVIST_DATA_H
#define ARCHIVIST_DATA_H

#define LINE_SEPARATOR			'\n'

#define PTS_MAX					255

// 16b
struct data_point
{
	int64_t				ts;
	double				val;
};

// 2k
struct data_points
{
	PTS				*	next;
	int64_t				count;
	PNT					vals[PTS_MAX];
};





line_fn data_handle_line;
line_fn data_handle_record;

buf_fn data_parse_line;
buf_fn data_parse_bin;


#endif
