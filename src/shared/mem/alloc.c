/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem.c - memory control, free list management                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

static uint8_t mem_alloc_size_vals[16] =
{
	0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4
};

uint32_t mem_alloc_size( int len )
{
	uint32_t l = (uint32_t) len;
	uint8_t s = 0;

	if( l & 0xffff0000 )
		return len + 1;

	if( l & 0xff00 )
	{
		s += 8;
		l >>= 8;
	}
	if( l & 0xf0 )
	{
		s += 4;
		l >>= 4;
	}

	s += mem_alloc_size_vals[l];

	// minimum 16 bytes
	return 1 << ( ( s > 4 ) ? s : 4 );
}


void *mem_perm( uint32_t len )
{
	void *p;

	// ensure 4-byte alignment
	if( len & 0x3 )
		len += 4 - ( len % 4 );

	if( len >= ( _mem->perm->size >> 5 ) )
	{
		return allocz( len );
	}

	pthread_mutex_lock( &(_mem->perm->lock ) );

	if( len > _mem->perm->left )
	{
		_mem->perm->space = allocz( _mem->perm->size );
		_mem->perm->curr  = _mem->perm->space;
		_mem->perm->left  = _mem->perm->size;
	}

	p = _mem->perm->curr;

#ifdef __GNUC__
#if __GNUC_PREREQ(4,8)
#pragma GCC diagnostic ignored "-Wpointer-arith"
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#endif
	_mem->perm->curr += len;
#ifdef __GNUC__
#if __GNUC_PREREQ(4,8)
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif
#endif
	_mem->perm->left -= len;

	pthread_mutex_unlock( &(_mem->perm->lock) );

	return p;
}



/*
 * This is a chunk of memory we hand back if calloc failed.
 *
 * The idea is that we hand back this highly distinctive chunk
 * of space.  Any pointer using it will fail.  Any attempt to
 * write to it should fail.
 *
 * So it should give us a core dump at once, right at the
 * problem.  Of course, threads will "help"...
 */

#ifdef USE_MEM_SIGNAL_ARRAY
static const uint8_t mem_signal_array[128] = {
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
	0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0
};
#endif


// zero'd memory
void *allocz( size_t size )
{
	//void *p = calloc( 1, size );
	void *p = malloc( size );
	memset( p, 0, size );

#ifdef USE_MEM_SIGNAL_ARRAY
	if( !p )
		p = &mem_signal_array;
#endif

	return p;
}


