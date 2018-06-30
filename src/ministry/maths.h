/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* sort.h - defines mathematical functions and structures                  *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_MATHS_H
#define MINISTRY_MATHS_H


#define DEFAULT_MATHS_PREDICT		30
#define MAX_MATHS_PREDICT			500
#define MIN_MATHS_PREDICT			10


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


#endif
