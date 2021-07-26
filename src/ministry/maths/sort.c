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
* maths/sort.c - sorting functions                                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"


/*
 *  In the old days, ministry just did qsort.  This is all well and good, but
 *  then some uses took to providing 100k+ values on a single metric in a 10s
 *  interval, and qsort's limits were starting to show.
 *
 *  A bit of investigation turned up a radix sort implementation, and also
 *  source code for glibc qsort (which mixed comparisons and insertion sort).
 *
 *  Some hacking later and qsort had been simplified to work exclusively on
 *  arrays of doubles, with the happy feature that the cmp() function could be
 *  replaced with a simple a < b check, eliminating a lot of overhead.  Also
 *  the generic version uses char *'s to avoid sizing concerns, but then also
 *  has to handle the object size.  This implementation just uses double *'s
 *  thoughtout.
 *
 *  Some benchmarking suggested that dbl_qsort was faster for small numbers
 *  of points - under 10,000.  Around that point the two algorithms were more
 *  or less equal.  Above that, radix sort seemed to win.  So there's a simple
 *  test - count < 10k, dbl_qsort, otherwise radix.
 *
 *  So both functions are here, along with a wrapper for qsort.  Radix sort
 *  needs another workspace, and some counter space, so those are provided.
 */




/*
 *	LIBC QSORT
 *	Simple, effective, well-known
 */
static inline int sort_cmp_floats( const void *p1, const void *p2 )
{
	double *d1 = (double *) p1;
	double *d2 = (double *) p2;

	return ( *d1 > *d2 ) ?  1 :
		   ( *d1 < *d2 ) ? -1 : 0;
}

// sorter_fn
void sort_qsort_glibc( ST_THR *t, int32_t ct )
{
	qsort( t->wkspc, ct, sizeof( double ), sort_cmp_floats );
}






/*
 *  RADIX SORT (11 bit), from
 *  https://bitbucket.org/ais/usort/src/474cc2a19224/usort/f8_sort.c
 *
 *  Copyright 2009 Andrew I Schein
 *
 *  Slightly modified to reduce handling of cases we don't need
 *  Really quite fast.
 */


#define _0(v) ( (v)         & 0x7FF)
#define _1(v) (((v)  >> 11) & 0x7FF)
#define _2(v) (((v)  >> 22) & 0x7FF)
#define _3(v) (((v)  >> 33) & 0x7FF)
#define _4(v) (((v)  >> 44) & 0x7FF)
#define _5(v) (((v)  >> 55) & 0x7FF)

#define X_counter( x )		b##x[_##x(buf1[n])]++
#define X_swapper( x )		tsum = b##x[j] + sum##x; b##x[j] = sum##x - 1; sum##x = tsum

//#define X_rdwrLoop( x )		for( n = 0; n < ct; ++n ) { pos = _##x(rdr[n]); wtr[++b##x[pos]] = rdr[n]; }
#define X_rdwrLoop( x )		for( n = 0; n < ct; ++n ) wtr[++b##x[_##x(rdr[n])]] = rdr[n];

#define X_rdr_wtr( x )		rdr = buf1; wtr = buf2; X_rdwrLoop( x )
#define X_wtr_rdr( x )		wtr = buf1; rdr = buf2; X_rdwrLoop( x )

static inline uint64_t f8_sort_FloatFlip( uint64_t u )
{
	uint64_t mask       =  -(u >> 63) | 0x8000000000000000ull ;
	return                  (u ^ mask);
}

static inline uint64_t f8_sort_IFloatFlip( uint64_t u )
{
	uint64_t mask       =  ((u >> 63) - 1) | 0x8000000000000000ull;
	return                  (u ^ mask);
}

// sorter_fn
void sort_radix11( ST_THR *t, int32_t ct )
{
	uint32_t sum0, sum1, sum2, sum3, sum4, sum5, tsum;
	uint32_t *b0, *b1, *b2, *b3, *b4, *b5, pos;
	uint64_t *rdr, *wtr, *buf1, *buf2;
	register int32_t n;
	int32_t j;

	sum0 = sum2 = sum4 = 0;
	sum1 = sum3 = sum5 = 0;

	buf1 = (uint64_t *) t->wkspc;
	buf2 = (uint64_t *) t->wkbuf2;

	// zero the counters section
	memset( t->counters, 0, 6 * F8_SORT_HIST_SIZE * sizeof( uint32_t ) );

	b0 = t->counters;
	b1 = b0 + F8_SORT_HIST_SIZE;
	b2 = b1 + F8_SORT_HIST_SIZE;
	b3 = b2 + F8_SORT_HIST_SIZE;
	b4 = b3 + F8_SORT_HIST_SIZE;
	b5 = b4 + F8_SORT_HIST_SIZE;

	for( n = 0; n < ct; ++n )
	{
		buf1[n] = f8_sort_FloatFlip( buf1[n] );

		X_counter( 0 );
		X_counter( 1 );
		X_counter( 2 );
		X_counter( 3 );
		X_counter( 4 );
		X_counter( 5 );
	}

	for( j = 0; j < F8_SORT_HIST_SIZE; ++j )
	{
		X_swapper( 0 );
		X_swapper( 1 );
		X_swapper( 2 );
		X_swapper( 3 );
		X_swapper( 4 );
		X_swapper( 5 );
	}

	X_rdr_wtr( 0 );
	X_wtr_rdr( 1 );
	X_rdr_wtr( 2 );
	X_wtr_rdr( 3 );
	X_rdr_wtr( 4 );

	// byte 5
	wtr = buf1;
	rdr = buf2;
	// slightly different
	for( n = 0; n < ct; ++n )
	{
		pos = _5(rdr[n]);
		wtr[++b5[pos]] = f8_sort_IFloatFlip( rdr[n] );
	}

	// no buffers to free (compared to original)
}

#undef X_counter
#undef X_swapper
#undef X_rdr_wtr
#undef X_wtr_rdr
#undef X_rdwrLoop

#undef _0
#undef _1
#undef _2
#undef _3
#undef _4
#undef _5





/*
 *  DOUBLES-ONLY QSORT, adapated from glibc qsort at
 *  https://code.woboq.org/userspace/glibc/stdlib/qsort.c.html
 *
 *  Uses the GNU libc source as inspiration and adapts it for doubles only
 *  to reduce overheads from being more generic
 */

typedef struct
{
	double	*	lo;
	double	*	hi;
} stack_node;

/* The next 4 #defines implement a very fast in-line stack abstraction. */
/* The stack needs log (total_elements) entries (we could even subtract
   log(MAX_THRESH)).  Since total_elements has type size_t, we get as
   upper bound for log (total_elements):
   bits per byte (CHAR_BIT) * sizeof(size_t).  */
#define STACK_SIZE				(CHAR_BIT * sizeof(size_t))
#define PUSH(low, high)			top->lo = low; top->hi = high; ++top
#define POP(low, high)			--top; low = top->lo; high = top->hi
#define STACK_NOT_EMPTY			(stack < top)

#define MAX_THRESH				4
#define SWAP( _a, _b )			swtmp = *_a; *_a = *_b; *_b = swtmp

#define CMP( _a, _b )			( *_a < *_b )


/* Order size using quicksort.  This implementation incorporates
   four optimizations discussed in Sedgewick:

   1. Non-recursive, using an explicit stack of pointer that store the
      next array partition to sort.  To save time, this maximum amount
      of space required to store an array of SIZE_MAX is allocated on the
      stack.  Assuming a 32-bit (64 bit) integer for size_t, this needs
      only 32 * sizeof(stack_node) == 256 bytes (for 64 bit: 1024 bytes).
      Pretty cheap, actually.

   2. Chose the pivot element using a median-of-three decision tree.
      This reduces the probability of selecting a bad pivot value and
      eliminates certain extraneous comparisons.

   3. Only quicksorts TOTAL_ELEMS / MAX_THRESH partitions, leaving
      insertion sort to order the MAX_THRESH items within each partition.
      This is a big win, since insertion sort is faster for small, mostly
      sorted array segments.

   4. The larger of the two sub-partitions is always pushed onto the
      stack first, with the algorithm then concentrating on the
      smaller partition.  This *guarantees* no more than log (total_elems)
      stack size is needed (actually O(1) in this case)!  */


void sort_qsort_dbl_arr( double *arr, int32_t ct )
{
	double swtmp = 0.0;

	if( ct == 0 )
		return;

	if( ct > MAX_THRESH )
	{
		double *lo = arr;
		double *hi = lo + ( ct - 1 );
		stack_node stack[STACK_SIZE];
		stack_node *top = stack;

		PUSH( NULL, NULL );

		while( STACK_NOT_EMPTY )
		{
			double *left, *right, *mid;

			mid = lo + ( ( hi - lo ) >> 1 );

			if( CMP( mid, lo ) )
			{
				SWAP( mid, lo );
			}

			if( CMP( hi, mid ) )
			{
				SWAP( hi, mid );

				// check mid/lo again - hi might have been the lowest
				if( CMP( mid, lo ) )
				{
					SWAP( lo, mid );
				}
			}

			left  = lo + 1;
			right = hi - 1;

			/* Here's the famous ``collapse the walls'' section of quicksort.
			   Gotta like those tight inner loops!  They are the main reason
			   that this algorithm runs much faster than others. */
			do
			{
				while( CMP( left, mid ) )
					++left;

				while( CMP( mid, right ) )
					--right;

				if( left < right )
				{
					SWAP( left, right );

					if( mid == left )
						mid = right;
					else if( mid == right )
						mid = left;

					++left;
					--right;
				}
				else if( left == right )
				{
					++left;
					--right;
					break;
				}
			}
			while( left <= right );

			/* Set up pointers for next iteration.  First determine whether
			  left and right partitions are below the threshold size.  If so,
			  ignore one or both.  Otherwise, push the larger partition's
			  bounds on the stack and continue sorting the smaller one. */

			if( ( right - lo ) <= MAX_THRESH )
			{
				if( ( hi - left ) <= MAX_THRESH )
				{
					// ignore both small partitions
					POP( lo, hi );
				}
				else
					// ignore small left partition
					lo = left;
			}
			else if( ( hi - left ) < MAX_THRESH )
			{
				// ignore small right partition
				hi = right;
			}
			else if( ( right - lo ) > ( hi - left ) )
			{
				// push larger left partition indices
				PUSH( lo, right );
				lo = left;
			}
			else
			{
				// push larger right partition indices
				PUSH( left, hi );
				hi = right;
			}

		}
	}

	/*	Once the BASE_PTR array is partially sorted by quicksort the rest
		is completely sorted using insertion sort, since this is efficient
		for partitions below MAX_THRESH size. BASE_PTR points to the beginning
		of the array to sort, and END_PTR points at the very last element in
		the array (*not* one beyond it!). */

	{
		double *thresh, *run, *tmp = arr, *end = arr + ( ct - 1 );

		thresh = ( ( arr + MAX_THRESH ) < end ) ? arr + MAX_THRESH : end;

	/*	Find smallest element in first threshold and place it at the
		array's beginning.  This is the smallest array element,
		and the operation speeds up insertion sort's inner loop. */

		for( run = tmp + 1; run <= thresh; ++run )
			if( CMP( run, tmp ) )
				tmp = run;

		if( tmp != arr )
		{
			SWAP( tmp, arr );
		}

		/* Insertion sort, running from left-hand-side up to right-hand-side */

		run = arr + 1;

		while( ++run <= end )
		{
			tmp = run - 1;
			while( CMP( run, tmp ) )
				--tmp;

			if( ++tmp != run )
			{
				double *h, *l, d, *tr = run + 1;

				while( --tr >= run )
				{
					d = *tr;
					for( h = l = tr; --l >= tmp; h = l )
						*h = *l;

					*h = d;
				}
			}
		}
	}
}


// sorter fn
void sort_qsort_dbl( ST_THR *t, int32_t ct )
{
	sort_qsort_dbl_arr( t->wkspc, ct );
}


#undef STACK_SIZ
#undef PUSH
#undef POP
#undef STACK_NOT_EMPTY

#undef MAX_THRESH
#undef SWAP




