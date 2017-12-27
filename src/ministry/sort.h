/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* sort.h - defines sorting functions and structures                       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_SORT_H
#define MINISTRY_SORT_H

#define DEFAULT_QSORT_THRESHOLD				10000		// flipover for radix faster than qsort
#define MIN_QSORT_THRESHOLD					2048		// min for radix

#define F8_SORT_HIST_SIZE					2048		// 11 bits

void sort_qsort_glibc( ST_THR *t, int32_t ct );		// legacy
void sort_qsort_dbl( ST_THR *t, int32_t ct );		// faster below 10k
void sort_radix11( ST_THR *t, int32_t ct );			// faster above 10k

#endif
