/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* query/local.h - local query object structures                           *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef ARCHIVIST_QUERY_LOCAL_H
#define ARCHIVIST_QUERY_LOCAL_H


#define DEFAULT_QUERY_TIMESPAN			3600		// 1hr
#define DEFAULT_QUERY_MAX_PATHS			2500



#include "archivist.h"

extern QRFN query_function_defns[];

query_data_fn query_xform_scale;
query_data_fn query_xform_offset;
query_data_fn query_xform_abs;
query_data_fn query_xform_log;
query_data_fn query_xform_deriv;
query_data_fn query_xform_nn_deriv;
query_data_fn query_xform_integral;

int query_parse( QRY *q );


#endif
