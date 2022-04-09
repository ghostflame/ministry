/**************************************************************************
* Copyright 2015 John Denholm                                             *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
*                                                                         *
*                                                                         *
* query/local.h - local query object structures                           *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef ARCHIVIST_QUERY_LOCAL_H
#define ARCHIVIST_QUERY_LOCAL_H


#define DEFAULT_QUERY_TIMESPAN			3600		// 1hr
#define DEFAULT_QUERY_MAX_PATHS			2500
#define DEFAULT_QUERY_MAX_CURR			100			// concurrency limit


#include "archivist.h"



#define query_curr_lock( )				pthread_mutex_lock(   &(_qry->qlock) );
#define query_curr_unlock( )			pthread_mutex_unlock( &(_qry->qlock) );





extern QRFN query_function_defns[];

query_data_fn query_xform_scale;
query_data_fn query_xform_offset;
query_data_fn query_xform_abs;
query_data_fn query_xform_log;
query_data_fn query_xform_deriv;
query_data_fn query_xform_nn_deriv;
query_data_fn query_xform_integral;
query_data_fn query_xform_summarize;

query_data_fn query_combine_sum;
query_data_fn query_combine_average;
query_data_fn query_combine_presence;


int query_add_current( void );
void query_rem_current( void );


int query_parse( QRY *q );


#endif
