// floating point constants don't work in the CP/M 68K v1.3 C compiler.
// but atof works, so use that instead.

#include <stdio.h>

extern float atof();

//////////////////////////////////////

// this implementation is terrible, but better than rand() in this C runtime

#define LCG_MULTIPLIER ( (unsigned long) 1000003 )
#define LCG_INCREMENT ( (unsigned long) 30011 )

static unsigned long lcg_seed = 1000000123;

// Function to seed the LCG
int cr_seed( seed ) unsigned long seed; { lcg_seed = seed; }

unsigned long cr_rand()
{
    lcg_seed >>= 5;
    lcg_seed = LCG_MULTIPLIER * lcg_seed + LCG_INCREMENT;
    return 0x7fffffff & lcg_seed;
}

//////////////////////////////////////

unsigned long __min( a, b ) unsigned long a; unsigned long b;
{
    if ( a > b )
        return a;
    return b;
}

unsigned long __max( a, b ) unsigned long a; unsigned long b;
{
    if ( a < b )
        return a;
    return b;
}

unsigned long gcd( m, n ) unsigned long m; unsigned long n;
{
    unsigned long a = 0;
    unsigned long b = __max( m, n );
    unsigned long r = __min( m, n );

    while ( 0 != r )
    {
        a = b;
        b = r;
        r = a % b;
    }

    return b;
} //gcd

#if 0 // unused

unsigned long rnd()
{
    return rand();
}

unsigned long ulrand()
{
    unsigned long ul;
    ul = rnd() | ( rnd() << 8 ) | ( rnd() << 16 ) | ( rnd() << 24 );
    return ul & 0x7fffffff;
} //randul

float itof( i ) int i;
{
    char ac[ 20 ];
    sprintf( ac, "%u", i );
    return atof( ac );
}

float ltof( l ) long l;
{
    char ac[ 20 ];
    sprintf( ac, "%lu", l );
    return atof( ac );
}

#endif

// https://en.wikipedia.org/wiki/Ap%C3%A9ry%27s_theorem

void first_implementation()
{
    unsigned long total = 10000;
    float sofar = atof( "0.0" );
    unsigned long prev = 1;
    long i;
    float fone = atof( "1.0" );
    float f;

    for ( i = 1; i <= total; i++ )
    {
        // do multiply with floats not longs to avoid overflow by 10000 iterations
        f = (float) i;
        sofar += fone / ( f * f * f );

        if ( i == ( prev * 10 ) )
        {
            prev = i;
            printf( "  at %12lu iterations: %.20f\n", i, sofar );
        }
    }
} //first_implementation

int main()
{
    long totalEntries = 10000;
    long totalCoprimes = 0;
    long prev = 1;
    long i;
    unsigned long a, b, c, greatest;
    float f, tc;
    printf( "starting, should tend towards 1.2020569031595942854...\n" );

    first_implementation();

    printf( "next implementation...\n" );

    // this version doesn't converge well because rand() doesn't work well

    for ( i = 1; i <= totalEntries; i++ )
    {
        a = cr_rand();
        b = cr_rand();
        c = cr_rand();

        greatest = gcd( a, gcd( b, c ) );
        if ( 1 == greatest )
            totalCoprimes++;

        if ( i == ( prev * 10 ) )
        {
            prev = i;
            f = (float) i / (float) totalCoprimes;
            printf( "  at %12ld iterations: %.20f\n", i, f );
        }
    }

    printf( "tap completed with great success\n" );
    exit( 1202 );
    return 1202;
} //main
