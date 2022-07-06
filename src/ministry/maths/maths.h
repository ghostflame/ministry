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
* maths/maths.h - defines maths/history/sorting functions and structures  *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_MATHS_H
#define MINISTRY_MATHS_H


#define DEFAULT_MATHS_PREDICT				30
#define MAX_MATHS_PREDICT					500
#define MIN_MATHS_PREDICT					10


#define DEFAULT_QSORT_THRESHOLD				10000		// flipover for radix faster than qsort
#define MIN_QSORT_THRESHOLD					2048		// min for radix

#define F8_SORT_HIST_SIZE					2048		// 11 bits




struct history_data_point
{
	double				ts;
	double				val;
};


#define dp_set( _dp, t, v )			_dp.ts = t; _dp.val = v
#define dp_get_t( _dp )				_dp.ts
#define dp_get_v( _dp )				_dp.val

#define dpp_set( __dp, t, v )		__dp->ts = t; __dp->val = v
#define dpp_get_t( __dp )			__dp->ts
#define dpp_get_v( __dp )			__dp->val

struct history
{
	HIST			*	next;
	DPT				*	points;
	uint16_t			size;
	uint16_t			curr;
};



// one of these lives on the DHASH if we are
// doing predictions for it
struct maths_prediction
{
	PRED			*	next;
	HIST			*	hist;
	DPT					prediction;
	double				a;		// presumes a + bx + cx^2 ...
	double				b;
//	double				c;
//	double				d;
	double				fit;	// quality coefficient
	uint16_t			vcount;
	uint16_t			pcount;
	uint8_t				valid;
	uint8_t				pflag;
};


struct maths_moments
{
	double			*	input;
	int64_t				count;
	double				mean;

	double				sdev;
	double				skew;
	double				kurt;

	uint8_t				mean_set;
};


// I'll do more at some point
void maths_predict_linear( DHASH *d, ST_PRED *sp );

// see https://en.wikipedia.org/wiki/Kahan_summation_algorithm
void maths_kahan_summation( double *list, int len, double *sum );
void maths_moments( MOMS *m );

// sorting functions
void sort_qsort_glibc( ST_THR *t, int32_t ct );		// legacy
void sort_qsort_dbl( ST_THR *t, int32_t ct );		// faster below 10k
void sort_radix11( ST_THR *t, int32_t ct );			// faster above 10k

void sort_qsort_dbl_arr( double *arr, int32_t ct );	// fn exposed for histogram bounds

// history functions
#define history_get_newest( _h )	( _h->points + _h->curr )
#define history_get_oldest( _h )	( _h->points + ( ( _h->curr + 1 ) % _h->size ) )

int history_fetch_sorted( HIST *h, HIST *into );
void history_add_point( HIST *h, int64_t tval, double val );
HIST *history_create( uint16_t size );

#endif
