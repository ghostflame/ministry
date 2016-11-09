/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* token.h - defines token handling functions and structures               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_TOKEN_H
#define MINISTRY_TOKEN_H

#define DEFAULT_TOKEN_LIFETIME		2000	// msec
#define DEFAULT_TOKEN_MASK			0xff	// all types

#define TOKEN_TYPE_STATS			0x01
#define TOKEN_TYPE_ADDER			0x02
#define TOKEN_TYPE_GAUGE			0x04

#define TOKEN_TYPE_ALL				0xff


struct token_data
{
	TOKEN			*	next;

	int64_t				expires;

	int64_t				stats;
	int64_t				adder;
	int64_t				gauge;

	uint32_t			ip;
	int32_t				burned;

	int64_t				id;
};


struct token_info
{
	TOKEN			**	hash;
	int					hsize;
	int					enable;
	int64_t				lifetime;
	int64_t				mask;
};

void token_burn( TOKEN *t );

TOKEN *token_find( uint32_t ip, int type, int64_t val );
TOKEN *token_generate( uint32_t ip, int types );

throw_fn token_loop;

void token_init( void );
TOKENS *token_setup( void );

#endif
