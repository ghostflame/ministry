/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* synth/synth.c - synthetic metrics functions and config                  *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


static inline int __synth_set_first( SYNTH *s )
{
	int i;

	for( i = 0; i < s->parts; ++i )
		if( s->dhash[i]->proc.count > 0 )
		{
			s->target->proc.total = s->dhash[i]->proc.total;
			break;
		}

	return i + 1;
}


void synth_sum( SYNTH *s )
{
	int i;

	s->target->proc.total = 0;

	for( i = 0; i < s->parts; ++i )
		s->target->proc.total += s->dhash[i]->proc.total;

	s->target->proc.total *= s->factor;
}

void synth_diff( SYNTH *s )
{
	s->target->proc.total = s->factor * ( s->dhash[0]->proc.total - s->dhash[1]->proc.total );
}

void synth_div( SYNTH *s )
{
	DHASH *a, *b;

	a = s->dhash[0];
	b = s->dhash[1];

	if( b->proc.total == 0 )
		s->target->proc.total = 0;
	else
		s->target->proc.total = ( a->proc.total * s->factor ) / b->proc.total;
}

void synth_prod( SYNTH *s )
{
	int i;

	s->target->proc.total = s->factor;

	// only use present values
	for( i = 0; i < s->parts; ++i )
		if( s->dhash[i]->proc.count > 0 )
			s->target->proc.total *= s->dhash[i]->proc.total;
}

void synth_cap( SYNTH *s )
{
	DHASH *a, *b;

	a = s->dhash[0];
	b = s->dhash[1];

	s->target->proc.total = ( a->proc.total < b->proc.total ) ? a->proc.total : b->proc.total;
}

void synth_max( SYNTH *s )
{
	int i;

	i = __synth_set_first( s );

	for( ; i < s->parts; ++i )
		if( s->target->proc.total < s->dhash[i]->proc.total )
			s->target->proc.total = s->dhash[i]->proc.total;

	s->target->proc.total *= s->factor;
}

void synth_min( SYNTH *s )
{
	int i;

	i = __synth_set_first( s );

	for( ; i < s->parts; ++i )
		if( s->target->proc.total > s->dhash[i]->proc.total )
			s->target->proc.total = s->dhash[i]->proc.total;

	s->target->proc.total *= s->factor;
}

void synth_spread( SYNTH *s )
{
	double min, max;
	int i;

	i = __synth_set_first( s );

	min = max = s->target->proc.total;

	for( ; i < s->parts; ++i )
	{
		if( max < s->dhash[i]->proc.total )
			max = s->dhash[i]->proc.total;
		if( min > s->dhash[i]->proc.total )
			min = s->dhash[i]->proc.total;
	}

	s->target->proc.total = s->factor * ( max - min );
}

void synth_mean( SYNTH *s )
{
	synth_sum( s );
	s->target->proc.total /= s->parts;
}

void synth_meanIf( SYNTH *s )
{
	synth_sum( s );
	s->target->proc.total /= ( s->parts - s->absent );
}

void synth_count( SYNTH *s )
{
	s->target->proc.total = s->factor * ( s->parts - s->absent );
}

void synth_active( SYNTH *s )
{
	// we only get here if we had one active
	s->target->proc.total = 1;
}


