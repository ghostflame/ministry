/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* token.c - handles tokens                                                *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "ministry.h"

inline int64_t token_gen_value( void )
{
	int tries = 10;
	int64_t v = 0;

	// generate a token value
	while( v == 0 && tries-- > 0 )
		v = random( );

	// ugh, we must not return 0
	if( v == 0 )
		v = tsll( ctl->curr_time );

	return v;
}


void token_burn( TOKEN *t )
{
	t->burned = 1;
}


TOKEN *token_find( uint32_t ip, int type, int64_t val )
{
	TOKENS *ts = ctl->net->tokens;
	int64_t tval;
	TOKEN *t;
	int hval;

	// zero is never, ever valid
	if( !val )
		return NULL;

	// we hash on ip address
	hval = ip % ts->hsize;

	// search under lock
	lock_tokens( );

	for( t = ts->hash[hval]; t; t = t->next )
	{
		// wrong IP or already burned
		if( t->ip != ip || t->burned )
			continue;

		switch( type )
		{
			case TOKEN_TYPE_STATS:
				tval = t->stats;
				break;
			case TOKEN_TYPE_ADDER:
				tval = t->adder;
				break;
			case TOKEN_TYPE_GAUGE:
				tval = t->gauge;
				break;
			default:
				unlock_tokens( );
				return NULL;
		}

		// if this matches, that is it
		if( tval == val )
			break;
	}

	unlock_tokens( );
	return t;
}


static int64_t token_id = 0;

TOKEN *token_generate( uint32_t ip, int types )
{
	TOKENS *ts = ctl->net->tokens;
	TOKEN *t;
	int hval;

	t = mem_new_token( );
	t->ip = ip;

	// NOT THREAD SAFE
	t->id = token_id++;

	// hash on IP address
	hval = t->ip % ts->hsize;

	// mask our types
	types &= ts->mask;

	if( types & TOKEN_TYPE_STATS )
		t->stats = token_gen_value( );
	if( types & TOKEN_TYPE_ADDER )
		t->adder = token_gen_value( );
	if( types & TOKEN_TYPE_GAUGE )
		t->gauge = token_gen_value( );

	// and set the expires time
	t->expires = ts->lifetime + tsll( ctl->curr_time );

	// and insert
	lock_tokens( );

	t->next = ts->hash[hval];
	ts->hash[hval] = t;

	unlock_tokens( );

	return t;
}







void token_table_purge( int64_t now, int pos, TOKEN **flist )
{
	TOKEN *t, *prev, *next;
	TOKENS *ts;
	int lock;

	ts = ctl->net->tokens;

	prev = NULL;
	lock = 0;

	for( t = ts->hash[pos]; t; t = next )
	{
		next = t->next;

		// ignore active ones
		if( !t->burned && now < t->expires )
		{
			prev = t;
			continue;
		}

		// all changes happen under lock
		if( !lock )
		{
			lock_tokens( );
			lock = 1;
		}

		// fix any previous pointer
		if( prev )
			prev->next = next;
		else
			ts->hash[pos] = next;

		// add that to the free list
		t->next = *flist;
		*flist  = t;
	}

	if( lock )
		unlock_tokens( );
}



void token_purge( int64_t tval, void *arg )
{
	TOKENS *ts = ctl->net->tokens;
	TOKEN *free = NULL;
	int i;

	info( "Token purge." );

	for( i = 0; i < ts->hsize; i++ )
		token_table_purge( tval, i, &free );

	{
		struct in_addr ina;
		TOKEN *t;

		for( t = free; t; t = t->next )
		{
			ina.s_addr = t->ip;
			info( "Purging token: %ld: %s @ %ld",
				t->id,
				inet_ntoa( ina ),
				t->expires );
		}
	}


	if( free )
		mem_free_token_list( free );
}



void *token_loop( void *arg )
{
	THRD *t = (THRD *) arg;

	// loop every second, about, purging tokens
	if( ctl->net->tokens->enable )
		loop_control( "token purge", &token_purge, NULL, 1010203, 0, 0 );

	free( t );
	return NULL;
}





void token_init( void )
{
	TOKENS *ts = ctl->net->tokens;

	if( ts->enable )
	{
		ts->hash      = (TOKEN **) allocz( ts->hsize * sizeof( TOKEN * ) );
		// convert to nsec
		ts->lifetime *= 1000000;
	}
}


TOKENS *token_setup( void )
{
	TOKENS *ts;

	ts           = (TOKENS *) allocz( sizeof( TOKENS ) );
	ts->hsize    = hash_size( "tiny" );
	ts->lifetime = DEFAULT_TOKEN_LIFETIME;
	// off by default
	ts->enable   = 0;

	// all types by default
	ts->mask     = DEFAULT_TOKEN_MASK;

	return ts;
}

