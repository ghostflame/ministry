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
* hash.c - hashing functions                                              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "carbon_copy.h"


/*
 * Note:
 *
 * Carbon-relay's native 'carbon_ch' uses MD5.
 *
 * It computes the MD5, then takes the highest 2 bytes
 * of the sum as a hash val.
 */



/* FNV
 * http://isthe.com/chongo/tech/comp/fnv/
 */

#define HASH_FNV32_PRIME	0x01000193	// 2^24 + 2^8 + 93
#define HASH_FNV32_SEED		0x811c9dc5

__attribute__((hot)) inline uint32_t hash_fnv1( const char *str, int32_t len )
{
	uint32_t hval = HASH_FNV32_SEED;
	uint8_t *p = (uint8_t *) str;

	while( len-- )
	{
		hval *= HASH_FNV32_PRIME;
		hval ^= (uint32_t) *p++;
	}

	return hval;
}


__attribute__((hot)) inline uint32_t hash_fnv1a( const char *str, int32_t len )
{
	register uint32_t hval = HASH_FNV32_SEED;
	register uint8_t *p = (uint8_t *) str;

	while( len-- )
	{
		hval ^= (uint32_t) *p++;
		hval *= HASH_FNV32_PRIME;
	}

	return hval;
}


// fast xor

static const uint32_t hash_cksum_primes[8] =
{
	2909, 3001, 3083, 3187, 3259, 3343, 3517, 3581
};


__attribute__((hot)) inline uint32_t hash_cksum( const char *str, int32_t len )
{
	register uint32_t *p, sum = 0xbeef;
	int32_t rem;

	rem = len & 0x3;
	len = len >> 2;
	p   = (uint32_t *) str;

	// a little unrolling for good measure
	while( len > 4 )
	{
		sum ^= *p++;
		sum ^= *p++;
		sum ^= *p++;
		sum ^= *p++;
		len -= 4;
	}

	// and the rest
	while( len-- > 0 )
		sum ^= *p++;

	// and capture the rest
	str = (char *) p;
	while( rem-- > 0 )
		sum += *str++ * hash_cksum_primes[rem];

	return sum;
}


