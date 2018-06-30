/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* history.h - defines data structures and functions for point history     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#ifndef MINISTRY_HISTORY_H
#define MINISTRY_HISTORY_H

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


#define history_get_newest( _h )	( _h->points + _h->curr )
#define history_get_oldest( _h )	( _h->points + ( ( _h->curr + 1 ) % _h->size ) )

int history_fetch_sorted( HIST *h, HIST *into );
void history_add_point( HIST *h, int64_t tval, double val );
HIST *history_create( uint16_t size );

#endif
