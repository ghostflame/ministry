/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* regexp.h - defines regex handling functions and structures              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_REGEXP_H
#define SHARED_REGEXP_H


enum regex_responses
{
	REGEX_MATCH = 0,
	REGEX_FAIL = 1
};


struct regex_entry
{
	RGX					*	next;
	regex_t					r;
	char				*	src;
	int						slen;
	int						ret;
	int64_t					tests;
	int64_t					matched;
};

struct regex_list
{
	RGX					*	list;
	RGX					*	last;
	int						count;
	int						fb;
};

// tidy up
void regex_list_destroy( RGXL *rl );

// create a list structure
RGXL *regex_list_create( int fallback_match );

// update the fallback behaviour
void regex_list_set_fallback( int fallback_match, RGXL *list );

// make a regex and add it to the list
int regex_list_add( char *str, int negate, RGXL *list );

// test a string against a regex list
int regex_list_test( char *str, RGXL *list );

#endif

