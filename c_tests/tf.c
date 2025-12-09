#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define _USE_MATH_DEFINES 
#include <math.h>
#include <float.h>
#include <stdint.h>

#if defined(__SIZEOF_INT128__)
typedef unsigned __int128 uint128_t;
typedef __int128 int128_t;
#endif

char *floattoa( char *buffer, double d, int precision )
{
    char * pbuf = buffer;

    if ( d < 0 )
    {
        *pbuf++ = '-';
        *pbuf = 0;
        d *= -1;
    }

    uint32_t whole = (uint32_t) (float) d;
    sprintf( pbuf, "%u", whole );

    if ( precision > 0 )
    {
        pbuf = buffer + strlen( buffer );
        *pbuf++ = '.';

        double fraction = d - whole;

        while ( precision > 0 )
        {
            fraction *= 10.0;
            whole = fraction;
            *pbuf++ = '0' + whole;

            fraction -= whole;
            precision--;
        }

        *pbuf = 0;
    }

    return buffer;
} //floattoa

// less than full precision because libc only provides this much precision in trig functions

#define TRIG_FLT_EPSILON 0.00002  /* 0.00000011920928955078 */
#define TRIG_DBL_EPSILON 0.000002 /* 0.00000000000000022204 */
#define TRIG_LDBL_EPSILON 0.0000000000000002 /* 0.0000000000000000000000000000000001925930 */

void check_same_f( const char * operation, float a, float b, float dbgval )
{
    float diff = a - b;
    float abs_diff = fabsf( diff );
    bool eq = ( abs_diff <= TRIG_FLT_EPSILON );
    if ( !eq )
    {
        printf( "operation %s: float %.20f is not the same as float %.20f\n", operation, a, b );
        printf( "  original value: %.20f\n", dbgval );
        exit( 0 );
    }
} //check_same_f

void check_same_d( const char * operation, double a, double b, double dbgval )
{
    double diff = a - b;
    double abs_diff = fabs( diff );
    bool eq = ( abs_diff <= TRIG_DBL_EPSILON );
    if ( !eq )
    {
        printf( "operation %s: double %.20lf is not the same as double %.20lf\n", operation, a, b );
        printf( "  original value: %.20lf\n", dbgval );
        exit( 0 );
    }
} //check_same_d

#if defined(__SIZEOF_INT128__)
const int max_N_Iterations = 17; // limited by factorial size for int128_t
int128_t factorial( int128_t n )
#else
const int max_N_Iterations = 10; // limited by factorial size for int64_t
int64_t factorial( int64_t n ) // should work for up to n=20
#endif
{
    if ( 0 == n )
        return 1;

    return n * factorial( n - 1 );
} //factorial

double my_sin_d( double x, int n = max_N_Iterations )
{
    double result = 0;
    int sign = 1;

    for ( int64_t i = 1; i <= n; i++ ) 
    {
        result += sign * pow( x, ( 2 * i - 1 ) ) / factorial( 2 * i - 1 );
        sign *= -1;
    }

    return result;
} //my_sin_d

float my_sin_f( float x, int n = max_N_Iterations )
{
    float result = 0;
    int sign = 1;

    for ( int64_t i = 1; i <= n; i++ ) 
    {
        result += sign * powf( x, ( 2 * i - 1 ) ) / factorial( 2 * i - 1 );
        sign *= -1;
    }

    return result;
} //my_sin_f

void many_trigonometrics()
{
    long double ldresult, ldback;
    double dresult, dback;
    float fresult, fback;
    float f = 0.01 - ( M_PI / 2 ); // want to be >= negative half pi.

    //printf( "float epsilon: %.40lf\n", (double) FLT_EPSILON );
    //printf( "double epsilon: %.40lf\n", (double) DBL_EPSILON );
    //printf( "long double epsilon: %.40lf\n", (double) LDBL_EPSILON );

    while ( f < ( M_PI / 2 ) )
    {
        fresult = tanf( f );
        fback = atanf( fresult );
        check_same_f( "tan", f, fback, fresult );

        dresult = tan( (double) f );
        dback = atan( dresult );
        check_same_d( "tan", (double) f, dback, fresult );

        fresult = sinf( f );
        fback = my_sin_f( f );
        check_same_f( "sin vs my_sin", fresult, fback, f );

        fresult = sinf( f );
        fback = asinf( fresult );
        check_same_f( "sin", f, fback, fresult );

        fresult = my_sin_f( f );
        fback = asinf( fresult );
        check_same_f( "my sin", f, fback, fresult );

        dresult = sin( (double) f );
        dback = asin( dresult );
        check_same_d( "sin", (double) f, dback, dresult );

        dresult = my_sin_d( (double) f );
        dback = asin( dresult );
        check_same_d( "my sin", (double) f, dback, dresult );

        float f_cos = f + ( M_PI / 2 );
        fresult = cosf( f_cos );
        fback = acosf( fresult );
        check_same_f( "cos", f_cos, fback, fresult );

        dresult = cos( (double) f_cos );
        dback = acos( dresult );
        check_same_d( "cos", (double) f_cos, dback, dresult );

        f += .01f;
    }
} //many_trignometrics

float square_root_f( float num ) 
{
    float x = num; 
    float y = 1;
    const float e =  10.0f * FLT_EPSILON;

    while ( ( x - y ) > e ) 
    {
        x = ( x + y ) / 2;
        y = num / x;
    }
    return x;
} //square_root_f

double square_root_d( double num ) 
{
    double x = num; 
    double y = 1;
    const double e =  10.0f * DBL_EPSILON;

    while ( ( x - y ) > e ) 
    {
        x = ( x + y ) / 2;
        y = num / x;
    }
    return x;
} //square_root_d

long double square_root_ld( long double num ) 
{
    long double x = num; 
    long double y = 1;

    // the x64os emulator uses double precision for long double, so using DBL_EPSILON here
    const long double e =  10.0f * DBL_EPSILON; // LDBL_EPSILON;

    while ( ( x - y ) > e ) 
    {
        x = ( x + y ) / 2;
        y = num / x;
    }
    return x;
} //square_root_ld

#pragma GCC optimize ("O0")

int fl_cl_test()
{
    float f1_1 = 1.1;
    float f1_8 = 1.8;
    float f;
    int32_t x;

    f = floor( f1_1 );
    x = (int32_t) f;
    printf( "floor of 1.1: %f == %d\n", f, x );

    f = ceil( f1_1 );
    x = (int32_t) f;
    printf( "ceil of 1.1: %f == %d\n", f, x );

    f = floor( -f1_8 );
    x = (int32_t) f;
    printf( "floor of -1.8: %f == %d\n", f, x );

    f = ceil( -f1_8 );
    x = (int32_t) f;
    printf( "ceil of -1.8: %f == %d\n", f, x );

    return 0;
}

extern "C" int main()
{
    char ac[ 100 ];
    
    floattoa( ac, -1.234567, 8 );
    printf( "float converted by floattoa: %s\n", ac );
    floattoa( ac, 1.234567, 8 );
    printf( "float converted by floattoa: %s\n", ac );
    floattoa( ac, 34.567, 8 );
    printf( "float converted by floattoa: %s\n", ac );

    printf( "float from printf: %f\n", 45.678 );

    float f1 = 1.0;
    float f2 = 20.2;
    float fm1 = -1.342;
    float fr = f2 * fm1;
    float fd = 1000.0 / 3.0;
    float fs = sqrtf( fd );

    printf( "division result: %f, square root %f\n", fd, fs );

    floattoa( ac, fr, 6 );
    printf( "float converted with floattoa: %s\n", ac );

    printf( "result of 20.2 * -1.342: %f\n", fr );

    double d = (double) fr;
    printf( "result of 20.2 * -1.342 as a double: %lf\n", d );

    float pi = 3.14159265358979323846264338327952884197169399375105820974944592307;
    float radians = pi / 180.0 * 30.0;
    printf( "pi in radians: %f\n", radians );

    float s = sinf( radians );
    printf( "sinf of 30 degress is %lf\n", s );

    float sh = sinhf( 0.5 );
    printf( "sinhf of 0.5 is %f\n", sh );

    float c = cosf( radians );
    printf( "cosf of 30 degrees is %lf\n", c );

    float ch = cosh( 0.5 );
    printf( "cosh of 0.5 (in radians) is %f\n", ch );

    float t = tanf( radians );
    printf( "tanf of 30 degrees is %lf\n", t );

    float f = atof( "1.0" );
    float at = atanf( f );
    printf( "atanf of %lf is %lf\n", f, at );

    at = atan2f( 0.3, 0.2 );
    printf( "atan2f of 0.3, 0.2 is %lf\n", at );

    c = acosf( 0.3 );
    printf( "acosf of 0.3 is %lf\n", c );

    s = asinf( 0.3 );
    printf( "asinf of 0.3 is %lf\n", s );

    f = tanhf( 2.2 );
    printf( "tanhf of 2.2 is %lf\n", s );
    
    f = logf( 0.3 );
    printf( "logf of 0.3: %lf\n", f );

    f = log10f( 300.0 );
    printf( "log10f of 300: %lf\n", f );
    
    int exponent;
    float mantissa = frexpf( pi, &exponent );
    printf( "pi has mantissa: %lf, exponent %d\n", mantissa, exponent );

    fl_cl_test();

    float b = 2.7;
    for ( float a = 2.0; a < 3.0; a += 0.1 )
    {
        if ( a > b )
            printf( "g," );
        if ( a >= b )
            printf( "ge," );
        if ( a == b )
            printf( "eq," );
        if ( a < b )
            printf( "l," );
        if ( a <= b )
            printf( "le," );
    }
    printf( "\n" );

    many_trigonometrics();

    for ( float f = 1.0f; f < 100.0f; f += 1.38f )
        check_same_f( "square root float", square_root_f( f ), sqrtf( f ), f );

    for ( double d = 1.0; d < 100.0; d += 1.38 )
        check_same_d( "square root double", square_root_d( d ), sqrt( d ), d );

    printf( "test tf completed with great success\n" );
    exit( 0 );
} //main


