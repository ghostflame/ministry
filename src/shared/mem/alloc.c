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
	void *p = calloc( 1, size );

#ifdef USE_MEM_SIGNAL_ARRAY
	if( !p )
		p = &mem_signal_array;
#endif

	return p;
}


