#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>

static uint8_t get_p2_size_vals[16] =
{
    0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4
};

static inline uint64_t get_p2_size( uint64_t in )
{
    int8_t bs = 0;

    if( in & 0xffffffff00000000 )
    {
        bs += 32;
        in >>= 32;
    }

    if( in & 0xffff0000 )
    {
        bs += 16;
        in >>= 16;
    }

    if( in & 0xff00 )
    {
        bs += 8;
        in >>= 8;
    }

    if( in & 0xf0 )
    {
        bs += 4;
        in >>= 4;
    }

    bs += get_p2_size_vals[in];
    return 1 << bs;
}


uint64_t vals[14] = {
	1, 3, 6, 7, 8, 20, 500, 1023, 1024, 4000, 4095, 4096, 4097, 10000
};

int main( int ac, char **av )
{
	struct timeval tv1, tv2;
	uint64_t a;
	double d;
	int i;

	for( i = 0; i < 14; i++ )
		printf( "%6lu -> %6lu\n", vals[i], get_p2_size( vals[i] ) );

	gettimeofday( &tv1, NULL );
	for( i = 0; i < 1000000; i++ )
		a = get_p2_size( 1234567 );
	gettimeofday( &tv2, NULL );

	d  = (double) ( tv2.tv_usec - tv1.tv_usec );
	d += 1000000.0 * (double) ( tv2.tv_sec - tv1.tv_sec );
	d /= 1000000.0;

	printf( "Average time: %f usec.\n", d );

	return 0;
}
