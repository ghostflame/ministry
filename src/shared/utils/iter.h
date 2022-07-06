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
