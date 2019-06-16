/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* strings/buf.c - routines handling string buf objects                    *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "shared.h"


// permanent string buffer
BUF *strbuf( uint32_t size )
{
	BUF *b = (BUF *) allocz( sizeof( BUF ) );
	size_t sz;

	if( size )
	{
		// make a little room
		if( size < 128 )
			size += 24;

		sz       = mem_alloc_size( size );
		b->space = (char *) allocz( sz );
		b->sz    = (uint32_t) sz;
	}

	b->buf = b->space;
	return b;
}

BUF *strbuf_resize( BUF *b, uint32_t size )
{
	size_t sz;
	char *old;

	if( !size )
		return NULL;

	if( !b )
		return strbuf( size );

	if( size < 128 )
		size += 24;

	sz = mem_alloc_size( size );

	if( sz > b->sz )
	{
		old = b->space;

		b->space  = (char *) allocz( sz );
		b->sz     = sz;
		b->buf    = b->space;

		if( b->len > 0 )
			memcpy( b->space, old, b->len );

		free( old );
	}

	b->len    = 0;
	b->buf[0] = '\0';

	return b;
}

BUF *strbuf_copy( BUF *b, char *str, int len )
{
	if( !len )
		len = strlen( str );

	if( !b )
		return strbuf_create( str, len );

	if( (uint32_t) len >= b->sz )
		strbuf_resize( b, len + 1 );

	memcpy( b->buf, str, len );
	b->len = len;
	b->buf[b->len] = '\0';

	return b;
}

BUF *strbuf_add( BUF *b, char *str, int len )
{
	if( !len )
		len = strlen( str );

	if( !b )
		return strbuf_create( str, len );

	if( (uint32_t) len > ( b->sz - b->len ) )
		len = b->sz - b->len - 1;

	memcpy( b->buf + b->len, str, len );
	b->len += len;
	b->buf[b->len] = '\0';

	return b;
}

BUF *strbuf_create( char *str, int len )
{
	int k, l;
	BUF *b;

	if( !len )
		len = strlen( str );
	if( !len )
		len = 1;

	for( k = 0, l = len; l > 0; k++ )
		l = l >> 1;

	b = strbuf( 1 << k );
	strbuf_add( b, str, len );

	return b;
}

