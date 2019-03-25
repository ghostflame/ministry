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



http_callback post_handle_data;
http_callback post_handle_init;
http_callback post_handle_finish;



int post_init( void );


#endif
