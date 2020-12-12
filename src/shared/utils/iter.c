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
* iter.c - a simple iterator from an arguments input                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"



ITER *iter_init( ITER *it, char *str, int len )
{
	WORDS *w;
	int j;

	if( !it )
		it = (ITER *) allocz( sizeof( ITER ) );

	it->step = 1;
	it->data = str_copy( str, len );
	it->dlen = len;

	w = &(it->w);

	strmwords( w, it->data, it->dlen, ' ' );

	// spot a range XX - YY [ZZ]
	if( ( w->wc == 3 || w->wc == 4 )
	 && !strcmp( w->wd[1], "-" )
	 && ( parse_number( w->wd[0], &(it->start),  NULL ) != NUM_INVALID )
	 && ( parse_number( w->wd[2], &(it->finish), NULL ) != NUM_INVALID ) )
	{
		if( it->start > it->finish )
		{
			it->val = it->finish;
			it->finish = it->start;
			it->start = it->val;
		}
		else
			it->val = it->start;

		// get the longest number
		j = snprintf( it->numbuf, 32, "%ld", it->finish );
		it->flen = j;
		j = snprintf( it->numbuf, 32, "%ld", it->start );
		if( j > it->flen )
			it->flen = j;

		it->max = it->flen;

		// so set the format
		snprintf( it->fmtbuf, 15, "%%0%huld", it->flen );

		if( w->wc == 4 )
		{
			if( parse_number( w->wd[3], &(it->step), NULL ) )
			{
				warn( "Invalid step value '%s' -- ignoring.", w->wd[3] );
				it->step = 1;
			}
			else
			{
				if( it->step == 0 )
				{
					warn( "Invalid step value 0 -- ignoring." );
					it->step = 1;
				}
				// no negative steps, we sorted start/finish anyway
				else
					it->step = llabs( it->step );
			}
		}

		it->numeric = 1;

		debug( "Iterator created numeric, from %ld to %ld, step %ld.",
			it->start, it->finish, it->step );
	}
	else
	{
		// get the longest word
		for( j = 0; j < w->wc; ++j )
			if( w->len[j] > it->max )
				it->max = w->len[j];
	}

	return it;
}


void iter_clean( ITER *it, int doFree )
{
	if( !it )
		return;

	if( it->data )
		free( it->data );

	if( doFree )
		free( it );
	else
		memset( it, 0, sizeof( ITER ) );
}


int iter_longest( ITER *it )
{
	if( !it )
		return -1;

	return it->max;
}


int iter_next( ITER *it, char **ptr, int *len )
{
	if( !it )
		return -1;

	if( it->numeric )
	{
		if( it->val > it->finish )
			return -1;

		*len = snprintf( it->numbuf, 32, it->fmtbuf, it->val );
		*ptr = it->numbuf;

		it->val += it->step;
		return 0;
	}

	if( it->curr >= it->w.wc )
		return -1;

	*len = it->w.len[it->curr];
	*ptr = it->w.wd[it->curr];

	++(it->curr);
	return 0;
}


