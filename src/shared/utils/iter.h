/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* iter.h - structures and declarations for simple iterator                *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#ifndef SHARED_ITER_H
#define SHARED_ITER_H

struct iterator
{
	char		*	data;

	WORDS			w;
	char			numbuf[32];
	char			fmtbuf[16];

	int64_t			start;
	int64_t			finish;
	int64_t			step;
	int64_t			val;

	int16_t			curr;
	int16_t			max;

	int16_t			dlen;
	uint8_t			flen;
	int8_t			numeric;
};

// init an interator from a set of space-delimited args
// either a list of words, or the special format
// <lower> - <upper> [<step>]   (numbers)
// which will make the iterator generate string
// values of the numbers
//
// if the ITER arg is null, one is allocated
ITER *iter_init( ITER *it, char *str, int len );

// tidy up an iterator
// either free it, or just clear it down
void iter_clean( ITER *it, int doFree );

// get the next value
int iter_next( ITER *it, char **ptr, int *len );

// get the longest value
int iter_longest( ITER *it );


#endif
