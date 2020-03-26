/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
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
