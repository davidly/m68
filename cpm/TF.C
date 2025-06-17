// This test app is heavily-tailored to the DR CP/M 68K v1.3 C compiler and runtime.
// Note that the v1.2 libf.a must be used because the v1.3 version has many bugs.
// But add the v1.3 version afterwards because it has some working functions not implemented in v1.2.
// Like this: m68 lo68.68k -r -u__optoff -o %1.68k s.o %1.o clib libf12.a libf.a
// Floating point constants don't work with this compiler, so atof() is used throughout

#include <stdio.h>

typedef unsigned char uint8_t;
typedef char int8_t; /* DR compiler fails with a syntax error when using "signed char" */
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned long uint32_t;
typedef long int32_t;
typedef int bool;
#define true 1
#define false 0

// these operate on floats. some don't exist and some don't work correctly as noted below.

float atof();
float pow();
float log();
float tan();
float atan();
float sin();
float asin();
float cos();
float acos();
float sqrt();
float tanh();
float log10();
float frexp();
float fabs();

float TRIG_FLT_EPSILON;
float M_PI;

char * floattoa( buffer, f, precision ) char *buffer; float f; int precision;
{
    int32_t whole;
    float fraction;
    char * pbuf = buffer;
    float ften = atof( "10" );

    if ( f < 0 )
    {
        *pbuf++ = '-';
        *pbuf = 0;
        f *= -1;
    }

    whole = (int32_t) f;
    sprintf( pbuf, "%lu", whole );

    if ( precision > 0 )
    {
        pbuf = buffer + strlen( buffer );
        *pbuf++ = '.';

        fraction = f - (float) whole;

        while ( precision > 0 )
        {
            fraction *= ften;
            whole = fraction;
            *pbuf++ = '0' + whole;

            fraction -= (float) (int32_t) whole;
            precision--;
        }

        *pbuf = 0;
    }

    return buffer;
} //floattoa

int check_same_f( operation, a, b ) char * operation; float a; float b;
{
    float diff = a - b;
    float abs_diff = fabs( diff );
    bool eq = ( abs_diff <= TRIG_FLT_EPSILON );
    if ( !eq )
        printf( "operation %s: float %.20f is not the same as float %.20f\n", operation, a, b );
} //check_same_f

int32_t factorial( n ) int32_t n;
{
    if ( 0 == n )
        return 1;

    return n * factorial( n - 1 );
} //factorial

float my_pow_f( base, exp ) float base; int32_t exp;
{
    int32_t i;
    float fone = atof( "1.0" );
    float result = fone;

    if ( exp < 0 )
    {
        base = fone / base;
        exp = -exp;
    }

    for ( i = 0; i < exp; ++i )
        result *= base;
    return result;
} //my_pow_f

float my_sin_f( x ) float x;
{
    int32_t n = 7;
    int32_t tmp, fbang;
    float fone = atof( "1.0" );
    float fmone = -fone;
    float sign = fone;
    int32_t i;
    float result = atof( "0.0" );

    for ( i = 1; i <= n; i++ ) 
    {
        tmp = 2 * i - 1;
        fbang = factorial( tmp );
        // pow() in the C runtime produces incorrect results
        result += sign * my_pow_f( x, tmp ) / fbang;
        sign *= fmone;
    }

    return result;
} //my_sin_f

float my_asin( x ) float x;
{
    int n;
    float term;
    float result = atof( "0" );
    float fone = atof( "1.0" );
    float fmone = -fone;
    float ftwo = atof( "2.0" );
    float fthree = atof( "3.0" );

    if ( fabs( x ) > fone )
        return result;

    term = x;
    for ( n = 0; n < 20; n++ )
    {
        result += term;
        term *= x * x * ( ftwo * n + fone ) / ( ftwo * n + ftwo ) * ( ftwo * n + fone ) / ( ftwo * n + fthree );
    }

    return result;
} //my_asin

float my_acos( x ) float x;
{
    float fzero = atof( "0" );
    float fone = atof( "1" );
    float ftwo = atof( "2" );
    float fmone = atof( "-1" );
    float low = fzero;
    float high = M_PI;
    float mid;
    float cos_mid;
    float closeEnough = atof( "1e-6" );

    if ( x < fmone || x > fone )
        return fzero;

    if ( x == fmone )
        return M_PI;
    if ( x == fzero )
        return M_PI / ftwo;
    if ( x == fone )
        return fzero;

    for ( ;; )
    {
        mid = ( low + high ) / ftwo;
        cos_mid = cos( mid );

        if ( fabs( x - cos_mid ) <= closeEnough )
            return mid;

        if ( cos_mid > x ) 
            low = mid;
        else
            high = mid;
    }

    return fzero;
} //my_acos

float my_atan2f( y, x ) float y; float x;
{
    float fzero = atof( "0" );
    float ftwo = atof( "2" );
    float ratio;

    if ( x == fzero )
    {
        if ( y > fzero )
            return M_PI / ftwo;

        if ( y < fzero )
            return -M_PI / ftwo;

        return fzero;
    }

    ratio = y / x;

    if ( x > fzero )
        return atan( ratio );

    if ( y >= fzero )
        return atan( ratio ) + M_PI;

    return atan( ratio ) - M_PI;
} //my_atan2f

void many_trigonometrics()
{
    float fresult, fback;
    float f = ( -M_PI / atof( "2" ) ) + 0x000001; // want to be >= negative half pi.
    float fpoint01 = atof( "0.01" );

    while ( f < ( M_PI / 2 ) )
    {
        fresult = tan( f );
        fback = atan( fresult );
        check_same_f( "tan", f, fback );

        fresult = sin( f );
        fback = my_asin( fresult );
        check_same_f( "sin", f, fback );

        fresult = my_sin_f( f );
        fback = my_asin( fresult );
        check_same_f( "my sin", f, fback );

        f += fpoint01;
    }
} //many_trignometrics

float my_sqrt_f( num ) float num;
{
    float fone = atof( "1" );
    float ftwo = atof( "2" );
    float x = num; 
    float y = fone;
    float e = atof( ".00002" );

    while ( ( x - y ) > e ) 
    {
        x = ( x + y ) / ftwo;
        y = num / x;
    }
    return x;
} //my_sqrt_f

float my_frexp( x, exp ) float x; int * exp;
{
    float fzero = atof( "0" );
    float fhalf = atof( "0.5" );
    float fone = atof( "1" );
    float ftwo = atof( "2" );
    float abs_x;
    int exponent = 0;

    if ( x == fzero )
    {
        *exp = 0;
        return fzero;
    }

    abs_x = fabs( x );
    
    while ( abs_x >= fone )
    {
        abs_x /= ftwo;
        exponent++;
    }
    
    while ( abs_x < fhalf )
    {
        abs_x *= ftwo;
        exponent--;
    }
    
    *exp = exponent;
    return ( x >= fzero ) ? abs_x : -abs_x;
} //my_frexp

int main()
{
    char ac[ 100 ];
    int exponent;
    float f1, f2, fm1, fr, fd, fs;
    float pi, radians, s, c, t, f, at, mantissa, b, fpointone, fthree, fonehundred, fonepoint38, a;
    float fone = atof( "1" );

    TRIG_FLT_EPSILON = atof( "0.00002" );  /* 0.00000011920928955078 would be better, but trig functions don't have that precision */
    M_PI = atof( "3.14159265" ); // this atof trashes stack if this string is long
    
    floattoa( ac, atof( "-1.234567" ), 8 );
    printf( "float converted by floattoa: %s\n", ac );
    floattoa( ac, atof( "1.234567" ), 8 );
    printf( "float converted by floattoa: %s\n", ac );
    floattoa( ac, atof( "34.567" ), 8 );
    printf( "float converted by floattoa: %s\n", ac );

    printf( "float from printf: %f\n", atof( "45.678" ) );

    f1 = atof( "1.0" );
    f2 = atof( "20.2" );
    fm1 = atof( "-1.342" );
    fr = f2 * fm1;
    fd = atof( "1000.0" ) / atof( "3.0" );
    fs = sqrt( fd );

    printf( "division result: %f, square root %f\n", fd, fs );

    floattoa( ac, fr, 6 );
    printf( "float converted with floattoa: %s\n", ac );

    printf( "result of 20.2 * -1.342: %f\n", fr );

    pi = M_PI;
    radians = pi / atof( "180.0" ) * atof( "30.0" );
    printf( "pi in radians: %f\n", radians );

    s = sin( radians );
    printf( "sin of 30 degress is %f\n", s );

    s = my_sin_f( radians );
    printf( "my_sin_f of 30 degress is %f\n", s );

    c = cos( radians );
    printf( "cos of 30 degrees is %f\n", c );

    t = tan( radians );
    printf( "tan of 30 degrees is %f\n", t );

    f = atof( "1.0" );
    at = atan( f );
    printf( "atan of %f is %f\n", f, at );

    // atan2 in the C runtime produces incorrect results
    at = my_atan2_f( atof( "0.3" ), atof( "0.2" ) );
    printf( "atan2 of 0.3, 0.2 is %f\n", at );

    // acos() in the C runtime produces incorrect results
    c = my_acos( atof( "0.3" ) );
    printf( "acos of 0.3 is %f\n", c );

    // asin() in the C runtime produces incorrect results
    s = my_asin( atof( "0.3" ) );
    printf( "asin of 0.3 is %f\n", s );

    f = tanh( atof( "2.2" ) );
    printf( "tanh of 2.2 is %f\n", s );
    
    f = log( atof( "0.3" ) );
    printf( "log of 0.3: %f\n", f );

    f = log10( atof( "300.0" ) );
    printf( "log10 of 300: %f\n", f );
    
    // frexp in the C runtime produces incorrect results
    mantissa = my_frexp( pi, &exponent );
    printf( "pi has mantissa: %f, exponent %d\n", mantissa, exponent );

    b = atof( "2.7" );
    fpointone = atof( "0.1" );
    fthree = atof( "3.0" );
    for ( a = atof( "2.0" ); a < fthree; a += fpointone )
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

    fonehundred = atof( "100.0" );
    fonepoint38 = atof( "1.38" );
    for ( f = fone; f < fonehundred; f += fonepoint38 )
        check_same_f( "square root float", my_sqrt_f( f ), sqrt( f ) );

    printf( "test tf completed with great success\n" );
    exit( 0 );
} //main


