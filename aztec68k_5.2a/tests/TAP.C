#include <stdio.h>

#ifndef __max
#define __max( a, b ) (a) > (b) ? a : b
#define __min( a, b ) (a) < (b) ? a : b
#endif

typedef unsigned long ulong;

ulong randu()
{
    static ulong r = 1234577;
    r >>= 2;
    r *= 41011;
    return r;
}

ulong gcd( m, n ) ulong m, n;
{
    ulong a = 0;
    ulong b = __max( m, n );
    ulong r = __min( m, n );

    while ( 0 != r )
    {
        a = b;
        b = r;
        r = a % b;
    }

    return b;
} 

/* https://en.wikipedia.org/wiki/Ap%C3%A9ry%27s_theorem */

void first_implementation()
{
    ulong total = 1000;
    ulong i, iq;
    double sofar = 0.0;
    ulong prev = 1;

    for ( i = 1; i <= total; i++ )
    {
        iq = i * i * i;
        sofar += (double) 1.0 / (double) ( iq );
        /*printf( "i, iq, and sofar: %lu, %lu, %.16lf\n", i, iq, sofar );*/

        if ( i == ( prev * 10 ) )
        {
            prev = i;
            printf( "  at %12lu iterations: %.16lf\n", i, sofar );
            fflush( stdout );
        }
    }
} 

int main()
{
    ulong totalEntries = 100000; 
    ulong i, prev, totalCoprimes, greatest, a, b, c;

    printf( "starting, should tend towards 1.2020569031595942854...\n" );

    printf( "first implementation...\n" );
    first_implementation();

    printf( "second implementation...\n" );

    totalCoprimes = 0;
    prev = 1;

    for ( i = 1; i <= totalEntries; i++ )
    {
        a = randu();
        b = randu();
        c = randu();

        greatest = gcd( a, gcd( b, c ) );
        if ( 1 == greatest )
            totalCoprimes++;

        if ( i == ( prev * 10 ) )
        {
            prev = i;
            printf( "  at %12lu iterations: %.16lf\n", i, (double) i / (double) totalCoprimes );
            fflush( stdout );
        }
    }

    printf( "done\n" );
    return 1202;
} 
