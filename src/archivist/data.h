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


line_fn data_handle_line;
line_fn data_handle_record;

buf_fn data_parse_line;
buf_fn data_parse_bin;


#endif
