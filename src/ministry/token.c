/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* token.c - handles tokens                                                *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "ministry.h"

int64_t token_gen_value( void )
{
	int tries = 10;
	int64_t v = 0;

	// generate a token value
	while( v == 0 && tries-- > 0 )
		v = random( );

	// ugh, we must not return 0
	if( v == 0 )
		v = tsll( ctl->proc->curr_time );

	return v;
}


void token_burn( TOKEN *t )
{
	t->burned = 1;
}


uint64_t token_hashval( uint32_t ip, int16_t type )
{
	uint64_t hval;

	// hash on IP address and type
	hval  = (uint64_t) type;
	hval  = hval << 33;
	hval += (uint64_t) ip; 

	// and hashed on token hashsize
	return hval % ctl->net->tokens->hsize;
}


TOKEN *token_find( uint32_t ip, int16_t type, int64_t val )
{
	TOKENS *ts = ctl->net->tokens;
	TOKEN *t;
	uint64_t hval;

	// zero is never, ever valid
	if( !val )
		return NULL;

	hval = token_hashval( ip, type );

	// search under lock - avoids problems with the purger
	// both the purge and the lookup happen in single-purpose
	// threads so we're not bothered if the sleep a bit
	lock_tokens( );

	for( t = ts->hash[hval]; t; t = t->next )
	{
		// wrong IP or already burned
		if( t->ip != ip || t->burned )
		{
			debug( "Ignoring token %ld, ip mismatch or burned.", t->id );
			continue;
		}

		debug( "Comparing nonce %ld with val %ld", t->nonce, val );

		// if this matches, that is it
		if( t->nonce == val )
			break;
	}

	debug( "Token pointer: %p", t );

	unlock_tokens( );
	return t;
}


static int64_t token_id = 0;

static char *token_type_names[4] = {
	"stats", "adder", "gauge", "weird"
};

TOKEN *__token_generate_type( uint32_t ip, int16_t type )
{
	TOKENS *ts = ctl->net->tokens;
	uint64_t hval;
	TOKEN *t;


	// create a new token and fill it in
	t = mem_new_token( );
	t->ip      = ip;
	t->id      = ++token_id; // NOT THREAD SAFE
	t->type    = type;
	t->expires = ts->lifetime + tsll( ctl->proc->curr_time );
	t->nonce   = token_gen_value( );

	switch( type )
	{
		case TOKEN_TYPE_STATS:
			t->name = token_type_names[0];
			break;
		case TOKEN_TYPE_ADDER:
			t->name = token_type_names[1];
			break;
			case TOKEN_TYPE_GAUGE:
			t->name = token_type_names[2];
			break;
		default:
			t->name = token_type_names[3];
			break;
	}

	hval = token_hashval( ip, type );

	// and insert
	lock_tokens( );

	t->next = ts->hash[hval];
	ts->hash[hval] = t;

	unlock_tokens( );

	debug( "Generated token %ld, type: %hd, ip: %u, nonce: %ld",
		t->id, type, t->ip, t->nonce );

	return t;
}



void token_generate( uint32_t ip, int16_t types, TOKEN **ptrs, int max, int *count )
{
	TOKEN *t;

	memset( ptrs, 0, max * sizeof( TOKEN * ) );
	*count = 0;

	if( !ctl->net->tokens->enable )
		return;

	types &= ctl->net->tokens->mask;

	if( types & TOKEN_TYPE_STATS )
	{
		t = __token_generate_type( ip, TOKEN_TYPE_STATS );
		ptrs[(*count)++] = t;
	}
	if( types & TOKEN_TYPE_ADDER )
	{
		t = __token_generate_type( ip, TOKEN_TYPE_ADDER );
		ptrs[(*count)++] = t;
	}
	if( types & TOKEN_TYPE_GAUGE )
	{
		t = __token_generate_type( ip, TOKEN_TYPE_GAUGE );
		ptrs[(*count)++] = t;
	}
}


int token_url_handler( uint32_t ip, char **bptr, int max, void *arg )
{
	int types, len, i, count;
	TOKEN *t, *tlist[8];
	char *buf = *bptr;

	types = TOKEN_TYPE_STATS|TOKEN_TYPE_ADDER|TOKEN_TYPE_GAUGE;
	token_generate( ip, types, tlist, 8, &count );

	len = snprintf( buf, max, "{" );

	// run down the tokens
	for( i = 0; i < count; i++ )
	{
		t = tlist[i];

		len += snprintf( buf + len, max - len, "\"%s\": %ld, ",
				t->name, t->nonce );
	}

	// chop off the trailing ", "
	// hand-crafting json is such a pain but all the C libs to
	// do it really, really suck.
	if( buf[len-1] == ' ' )
		len--;
	if( buf[len-1] == ',' )
		len--;

	len += snprintf( buf + len, max - len, "}\n" );

	return len;
}






void token_table_purge( int64_t now, uint64_t pos, TOKEN **flist )
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
	uint64_t i;

	for( i = 0; i < ts->hsize; i++ )
		token_table_purge( tval, i, &free );


	{
		struct in_addr ina;
		TOKEN *t;

		for( t = free; t; t = t->next )
		{
			ina.s_addr = t->ip;
			info( "Purging token: %ld: %s/%hd @ %ld",
				t->id,
				inet_ntoa( ina ),
				t->type,
				t->expires );
		}
	}

	if( free )
		mem_free_token_list( free );
}



void token_loop( THRD *t )
{
	// loop every second, about, purging tokens
	if( ctl->net->tokens->enable )
		loop_control( "token purge", &token_purge, NULL, 1010203, 0, 0 );
}





int token_init( void )
{
	TOKENS *ts = ctl->net->tokens;

	if( ts->enable )
	{
		ts->hash      = (TOKEN **) allocz( ts->hsize * sizeof( TOKEN * ) );
		// convert to nsec
		ts->lifetime *= 1000000;
	}

	if( ts->filter_name )
	{
		if( !( ts->filter = iplist_find( ts->filter_name ) ) )
		{
			fatal( "Could not find token filter list '%s'.", ts->filter_name );
			return -1;
		}

		iplist_explain( ts->filter, NULL, NULL, "Tokens", NULL );
	}

	http_add_handler( "/tokens", 1024, &token_url_handler, "Issues tokens.", NULL );
	return 0;
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

