/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* maths.c - mathematical functions                                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"




void maths_predict_linear( DHASH *d, ST_PRED *sp )
{
	double ts, diffx, diffy, meanx, meany, sumy, sumxy, sumxx, sumyy, xxyy, pval;
	PRED *p = d->predict;
	HIST *h = p->hist;
	uint16_t i;
	DPT *dp;

	// calculate the new value
	for( sumy = 0.0, i = 0; i < h->size; i++ )
		sumy += dp_get_v( h->points[i] );

	meany = sumy / (double) h->size;

	// average timestamp 
	dp = history_get_oldest( h );
	meanx = dpp_get_t( dp );
	dp = history_get_newest( h );
	meanx += dpp_get_t( dp );
	meanx /= 2.0;

	//debug( "Means: x - %f, y - %f", meanx, meany );

	sumxx = 0.0, sumyy = 0.0, sumxy = 0.0;
	for( i = 0; i < h->size; i++ )
	{
		dp = h->points + i;
		diffx = dpp_get_t( dp ) - meanx;
		diffy = dpp_get_v( dp ) - meany;

		//debug( "Diffs: x - %f, y - %f", diffx, diffy );

		sumxy += diffx * diffy;
		sumxx += diffx * diffx;
		sumyy += diffy * diffy;
	}

	//debug( "Sums: xx - %f, xy - %f, yy - %f", sumxx, sumxy, sumyy );

	// coefficients
	p->b = sumxy / sumxx;
	p->a = meany - ( p->b * meanx );

	//debug( "Coefs: a - %f, b - %f", p->a, p->b );
	xxyy = sumyy * sumxx;

	if( xxyy != 0.0 )
		p->fit = ( sumxy * sumxy ) / xxyy;
	else
		p->fit = 0.0;

	// now we are in business
	ts = dp_get_t( p->prediction );
	pval = p->a + ( p->b * ts );
	dp_set( p->prediction, ts, pval );
}



// an implementation of Kaham Summation
// https://en.wikipedia.org/wiki/Kahan_summation_algorithm
// useful to avoid floating point errors
static inline void maths_kahan_sum( double val, double *sum, double *low )
{
	double y, t;

	y = val - *low;     // low starts off small
	t = *sum + y;       // sum is big, y small, lo-order y is lost

	*low = ( t - *sum ) - y;// (t-sum) is hi-order y, -y recovers lo-order
	*sum = t;           // low is algebraically always 0
}

void maths_kahan_summation( double *list, int len, double *sum )
{
	double low = 0;
	int i;

	for( *sum = 0, i = 0; i < len; i++ )
		maths_kahan_sum( list[i], sum, &low );

	*sum += low;
}

// https://en.wikipedia.org/wiki/Standard_deviation#Estimation
// https://en.wikipedia.org/wiki/Skewness#Sample_skewness
// https://en.wikipedia.org/wiki/Kurtosis#Sample_kurtosis
void maths_moments( MOMS *m )
{
	double sdev, skew, kurt, dtmp, stmp, ktmp, diff, prod, ct;
	int64_t i;

	if( !m || !m->count )
		return;

	sdev = skew = kurt = 0;
	dtmp = stmp = ktmp = 0;

	ct = (double) m->count;

	if( !m->mean_set )
	{
		m->mean = 0;

		for( i = 0; i < m->count; i++ )
			m->mean += m->input[i];

		m->mean /= ct;
	}

	for( i = 0; i < m->count; i++ )
	{
		// diff from mean
		diff = m->input[i] - m->mean;
		prod = diff * diff;

		// stddev needs sum of squares of diffs
		maths_kahan_sum( prod, &sdev, &dtmp );

		// skewness needs third moment
		prod *= diff;
		maths_kahan_sum( prod, &skew, &stmp );

		// kurtosis needs fourth moment
		prod *= diff;
		maths_kahan_sum( prod, &kurt, &ktmp );
	}

	// complete the kahan sum
	sdev += dtmp;
	skew += stmp;
	kurt += ktmp;

	// we don't need corrected - we have the whole population
	sdev /= ct;
	kurt /= ct;

	// using Fisher-Pearson standardized moment with any decent count size
	// http://www.statisticshowto.com/skewness/
	if( m->count > 5 )
	{
		skew *= ct;
		skew /= (double) ( ( m->count - 1 ) * ( m->count - 2 ) );
	}
	else
		skew /= ct;

	// and sqrt the variance to get the std deviation
	sdev = sqrt( sdev );

	// normalize against the variance
	skew /= sdev * sdev * sdev;
	kurt /= sdev * sdev * sdev * sdev;
	// and subtract 3 from kurtosis
	kurt -= 3;

	m->sdev = sdev;
	m->skew = skew;
	m->kurt = kurt;
}


