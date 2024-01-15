/**************************************************************************
* Copyright 2015 John Denholm                                             *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
*                                                                         *
*                                                                         *
* strings/buf.c - routines handling string buf objects                    *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"


// permanent string buffer
BUF *strbuf( uint32_t size )
{
	BUF *b = (BUF *) allocz( sizeof( BUF ) );
	size_t sz;

	if( !size )
		size = 128;

	// make a little room
	if( size < 128 )
		size += 24;

	sz     = mem_alloc_size( size );
	b->buf = (char *) allocz( sz );
	b->sz  = (uint32_t) sz;

	return b;
}

void strbuf_free( BUF *b )
{
	if( !b )
		return;

	if( b->buf )
		free( b->buf );

	free( b );
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
		old = b->buf;

		b->buf = (char *) allocz( sz );
		b->sz  = sz;

		if( b->len > 0 )
		{
			memcpy( b->buf, old, b->len );
			// argh!  Leak!  Fixed.
			free( old );
		}
	}

	return b;
}


BUF *strbuf_copy( BUF *b, const char *str, int len )
{
	if( !len )
		len = strlen( str );

	if( !b )
		return strbuf_create( str, len );

	if( (uint32_t) len >= b->sz )
		strbuf_resize( b, len + 1 );

	memcpy( b->buf, str, len );
	b->len = len;

	buf_terminate( b );

	return b;
}

int strbuf_copymax( BUF *b, const char *str, int len )
{
	int max;

	if( !len )
		len = strlen( str );

	max = (int) ( b->sz - 1 );
	if( len > max )
		len = max;

	memcpy( b->buf, str, len );
	b->len = len;

	buf_terminate( b );

	return len;
}


BUF *strbuf_add( BUF *b, const char *str, int len )
{
	if( !len )
		len = strlen( str );

	if( !b )
		return strbuf_create( str, len );

	if( (uint32_t) len > ( b->sz - b->len ) )
		len = b->sz - b->len - 1;

	memcpy( b->buf + b->len, str, len );
	b->len += len;

	buf_terminate( b );

	return b;
}

BUF *strbuf_json( BUF *b, json_object *o, int done )
{
	const char *str;
	size_t l;

	if( !o || !b )
		return NULL;

	str = json_object_to_json_string_length( o, JSON_C_TO_STRING_SPACED, &l );

	if( (uint32_t) l > b->sz )
		strbuf_resize( b, l + 2 );

	b->len = (uint32_t) l;
	memcpy( b->buf, str, b->len );

	buf_terminate( b );

	if( done )
		json_object_put( o );

	return b;
}

BUF *strbuf_create( const char *str, int len )
{
	int k, l;
	BUF *b;

	if( !len )
		len = strlen( str );

	for( k = 0, l = len; l > 0; ++k )
		l = l >> 1;

	b = strbuf( 1 << k );
	strbuf_add( b, str, len );

	return b;
}


// keep the data from the end of the buffer
// put it at the start of the buffer
void strbuf_keep( BUF *b, int len )
{
    register uint32_t l = len;
    register char *p, *q;

    if( l > b->len )
        return;

    if( l > 0 )
    {
        p = b->buf + ( b->len - l );

        if( p > ( b->buf + l ) )
            memcpy( b->buf, p, l );
        else
        {
            q = b->buf;
            while( l-- ) {
                *q++ = *p++;
            }
        }
    }

    b->len = (uint32_t) len;
    buf_terminate( b );
}

