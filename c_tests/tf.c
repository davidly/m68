#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define _USE_MATH_DEFINES 
#include <math.h>
#include <float.h>
#include <stdint.h>

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
#define TRIG_DBL_EPSILON 0.00000002 /* 0.00000000000000022204 */
#define TRIG_LDBL_EPSILON 0.0000000000000002 /* 0.0000000000000000000000000000000001925930 */

void check_same_f( const char * operation, float a, float b )
{
    float diff = a - b;
    float abs_diff = fabsf( diff );
    bool eq = ( abs_diff <= TRIG_FLT_EPSILON );
    if ( !eq )
        printf( "operation %s: float %.20f is not the same as float %.20f\n", operation, a, b );
} //check_same_f

void check_same_d( const char * operation, double a, double b )
{
    double diff = a - b;
    double abs_diff = fabs( diff );
    bool eq = ( abs_diff <= TRIG_DBL_EPSILON );
    if ( !eq )
        printf( "operation %s: double %.20lf is not the same as double %.20lf\n", operation, a, b );
} //check_same_d

int64_t factorial( int64_t n )
{
    if ( 0 == n )
        return 1;

    return n * factorial( n - 1 );
} //factorial

double my_sin_d( double x, int n = 18 )
{
    double result = 0;
    int64_t sign = 1;

    for ( int64_t i = 1; i <= n; i++ ) 
    {
        result += sign * pow( x, ( 2 * i - 1 ) ) / factorial( 2 * i - 1 );
        sign *= -1;
    }

    return result;
} //my_sin_d

float my_sin_f( float x, int n = 18 )
{
    float result = 0;
    int64_t sign = 1;

    for ( int64_t i = 1; i <= n; i++ ) 
    {
        result += sign * powf( x, ( 2 * i - 1 ) ) / factorial( 2 * i - 1 );
        sign *= -1;
    }

    return result;
} //my_sin_f

void many_trigonometrics()
{
    float f = ( -M_PI / 2 ) + 0x000001; // want to be >= negative half pi.

    //printf( "float epsilon: %.40lf\n", (double) FLT_EPSILON );
    //printf( "double epsilon: %.40lf\n", (double) DBL_EPSILON );
    //printf( "long double epsilon: %.40lf\n", (double) LDBL_EPSILON );

    while ( f < ( M_PI / 2 ) )
    {
        float fresult = tanf( f );
        float fback = atanf( fresult );
        check_same_f( "tan", f, fback );

        double dresult = tan( (double) f );
        double dback = atan( dresult );
        check_same_d( "tan", (double) f, dback );

        fresult = sinf( f );
        fback = asinf( fresult );
        check_same_f( "sin", f, fback );

        fresult = my_sin_f( f );
        fback = asinf( fresult );
        check_same_f( "my sin", f, fback );

        dresult = sin( (double) f );
        dback = asin( dresult );
        check_same_d( "sin", (double) f, dback );

        dresult = my_sin_d( (double) f );
        dback = asin( dresult );
        check_same_d( "my sin", (double) f, dback );

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
    const long double e =  10.0f * LDBL_EPSILON;

    while ( ( x - y ) > e ) 
    {
        x = ( x + y ) / 2;
        y = num / x;
    }
    return x;
} //square_root_ld

#pragma GCC optimize ("O0")

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

    float c = cosf( radians );
    printf( "cosf of 30 degrees is %lf\n", c );

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
        check_same_f( "square root float", square_root_f( f ), sqrtf( f ) );

    for ( double d = 1.0; d < 100.0; d += 1.38 )
        check_same_d( "square root double", square_root_d( d ), sqrt( d ) );

    printf( "test tf completed with great success\n" );
    exit( 0 );
} //main


