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
* typedefs.h - major typedefs for other structures                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef ARCHIVIST_TYPEDEFS_H
#define ARCHIVIST_TYPEDEFS_H

typedef struct archivist_control    ARCH_CTL;
typedef struct network_control		NETW_CTL;
typedef struct memt_control         MEMT_CTL;
typedef struct tree_control         TREE_CTL;
typedef struct query_control        QRY_CTL;
typedef struct file_control         FILE_CTL;

typedef struct network_type_data	NWTYPE;

typedef struct query_path           QP;
typedef struct query_data           QRY;
typedef struct query_fn             QRFN;
typedef struct query_fn_call		QRFC;


// function types
typedef int query_data_fn ( QRY *q, PTL *in, PTL **out, int argc, void **argv );

#endif
