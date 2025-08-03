#include <stdint.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>
#include <cmath>
#include <limits>

#define _perhaps_inline __attribute__((noinline))

// -Ofast doesn't understand infinity so the compiler would emit this warning
// v14 of clang doesn't understand this
#pragma clang diagnostic ignored "-Wnan-infinity-disabled"

template <class T> inline T get_max( T a, T b )
{
    if ( a > b )
        return a;
    return b;
} //get_max

template <class T> inline T get_min( T a, T b )
{
    if ( a < b )
        return a;
    return b;
} //get_min

double set_double_sign( double d, bool sign )
{
    uint64_t val = sign ? ( ( * (uint64_t *) &d ) | 0x8000000000000000 ) : ( ( * (uint64_t *) &d ) & 0x7fffffffffffffff );
    return * (double *) &val;
} //set_double_sign

const char * tf( bool f )
{
    return f ? "true" : "false";
}

void _perhaps_inline show_num( double d )
{
    printf( "  %lf = %#llx, isnan %s, isinf %s, iszero %s, signbit %s\n",
            d, * (uint64_t *) &d,
            tf( std::isnan( d ) ), tf( std::isinf( d ) ),
            tf( 0.0 == d ),  tf( std::signbit( d ) ) );
} //show_num

template <class T> void _perhaps_inline cmp( T a, T b )
{
    bool gt = ( a > b );
    bool lt = ( a < b );
    bool eq = ( a == b );
    bool le = ( a <= b );
    bool ge = ( a >= b );
    printf( "  lt %d le %d eq %d ge %d gt %d\n", lt, le, eq, ge, gt );
} //cmp

template <class T> void _perhaps_inline minmax( T a, T b )
{
    T min = get_min( a, b );
    T max = get_max( a, b );
    printf( "  min %lf, max %lf\n", (double) min, (double) max );
} //minmax

double _perhaps_inline do_math( double a, double b )
{
    double r = 0.0;

    printf( "  in do_math()\n" );
    printf( "         a:" );
    show_num( a );
    printf( "         b:" );
    show_num( b );

    r = a * b;
    printf( "         *:" );
    show_num( r );

    r = a / b;
    printf( "         /:" );
    show_num( r );

    r = a + b;
    printf( "         +:" );
    show_num( r );

    r = a - b;
    printf( "         -:" );
    show_num( r );

    printf( "       cmp:" );
    cmp( a, b );

    printf( "    minmax:" );
    minmax( a, b );

    return r;
} //do_math

double zero = 0.0;
double neg_zero = set_double_sign( 0.0, true );
double pos_infinity = INFINITY;
double neg_infinity = set_double_sign( INFINITY, true );
double not_a_number = NAN;
double neg_not_a_number = set_double_sign( NAN, true );
double quiet_nan = std::numeric_limits<double>::quiet_NaN();
double signaling_nan = std::numeric_limits<double>::signaling_NaN();

double test_case( double d )
{
    double r = 0.0;
    r += do_math( d, 0.0 );
    r += do_math( 0.0, d );
    r += do_math( d, neg_zero );
    r += do_math( neg_zero, d );
    r += do_math( 3.0, d );
    r += do_math( d, 3.0 );
    r += do_math( -3.0, d );
    r += do_math( d, -3.0 );
    r += do_math( d, not_a_number );
    r += do_math( not_a_number, d );
    r += do_math( d, neg_not_a_number );
    r += do_math( neg_not_a_number, d );
    r += do_math( d, pos_infinity );
    r += do_math( pos_infinity, d );
    r += do_math( d, neg_infinity );
    r += do_math( neg_infinity, d );
    r += do_math( d, d );
    return r;
} //test_case

int main( int argc, char * argv[] )
{
    printf( "NAN: %#llx\n", * (uint64_t *) & not_a_number );
    printf( "-NAN: %#llx\n", * (uint64_t *) & neg_not_a_number );
    printf( "quiet NAN: %#llx\n", * (uint64_t *) & quiet_nan );
    printf( "signaling NAN: %#llx\n", * (uint64_t *) & signaling_nan );
    printf( "INFINITY: %#llx\n", * (uint64_t *) & pos_infinity );
    printf( "-INFINITY: %#llx\n", * (uint64_t *) & neg_infinity );
    printf( "0.0: %#llx\n", * (uint64_t *) & zero );
    printf( "-0.0: %#llx\n", * (uint64_t *) & neg_zero );

    printf( "testing with NAN:\n" );
    test_case( not_a_number );

    printf( "testing with -NAN:\n" );
    test_case( neg_not_a_number );

    printf( "testing with INFINITY:\n" );
    test_case( pos_infinity );

    printf( "testing with -INFINITY:\n" );
    test_case( neg_infinity );

    printf( "testing with 69:\n" );
    test_case( 69.0 );

    printf( "testing with -69:\n" );
    test_case( -69.0 );

    printf( "testing with 0.0:\n" );
    test_case( zero );

    printf( "testing with -0.0:\n" );
    test_case( neg_zero );

    printf( "nan test completed with great success\n" );
    return 0;
}
