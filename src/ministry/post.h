/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* post.h - defines HTTP post handling functions                           *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_POST_H
#define MINISTRY_POST_H


struct post_response
{
	POST			*	next;
	HOST			*	h;
};



int post_handle_buffer( POST *p, NET_TYPE *n );

post_fn post_handle_adder;
post_fn post_handle_stats;
post_fn post_handle_gauge;
post_fn post_handle_compat;



int post_init( void );


#endif
