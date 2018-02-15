/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* typedefs.h - major typedefs for other structures                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_TYPEDEFS_H
#define MINISTRY_TYPEDEFS_H

typedef struct mintest_control		MTEST_CTL;
typedef struct metric_control		MTRC_CTL;
typedef struct targets_control		TGTS_CTL;

typedef struct metric				METRIC;
typedef struct metric_group			MGRP;

// function types
typedef void update_fn ( METRIC * );
typedef void line_fn ( METRIC * );

#endif
