#include <stdio.h>

typedef unsigned long uint32_t;
typedef int bool;
#define true 1
#define false 0

long atol();

uint32_t ulsqrt( n ) uint32_t n;
{
    uint32_t low = 1;
    uint32_t high = n / 2;
    uint32_t result = 0;
    uint32_t mid;

    if ( n <= 1 )
        return n;

    while ( low <= high )
    {
        mid = low + ( ( high - low ) / 2 );
        if ( mid <= ( n / mid ) )
        {
            result = mid;
            low = mid + 1;
        }
        else
            high = mid - 1;
    }

    return result;
}

int main( argc, argv ) int argc; char * argv[];
{
    uint32_t start = 10000;
    uint32_t num_found = 0;
    uint32_t s, i;
    bool is_prime;

    if ( argc >= 2 )
        start = atol( argv[ 1 ] );

    if ( 0 == ( start & 1 ) )
        start++;

    while ( num_found < 10 )
    {
        s = 1 + ulsqrt( start );
        is_prime = true;

        for ( i = 3; i < s; i += 2 )
        {
            if ( 0 == ( start % i ) )
            {
                is_prime = false;
                break;
            }
        }

        if ( is_prime )
        {
            num_found++;
            printf( "%lu\n", start );
        }

        start += 2;
    }

    return 0;
}
