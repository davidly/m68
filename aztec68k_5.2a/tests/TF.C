#include <stdio.h>
#include <math.h>

typedef unsigned char uint8_t;
typedef char int8_t; /* DR compiler fails with a syntax error when using "signed char" */
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned long uint32_t;
typedef long int32_t;
typedef int bool;
#define true 1
#define false 0

#define flt( x ) ( atof( #x ) )

float TRIG_FLT_EPSILON;
float M_PI;

char * floattoa( buffer, f, precision ) char *buffer; float f; int precision;
{
    int32_t whole;
    float fraction;
    char * pbuf = buffer;
    float ften = 10.0;
    float fzero = 0.0;
    float fmone = -1.0;

    if ( f < fzero )
    {
        *pbuf++ = '-';
        *pbuf = 0;
        f *= fmone;
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
} 

int check_same_f( operation, a, b ) char * operation; float a; float b;
{
    float diff = a - b;
    float abs_diff = fabs( diff );
    bool eq = ( abs_diff <= TRIG_FLT_EPSILON );
    if ( !eq )
        printf( "operation %s: float %.20f is not the same as float %.20f\n", operation, a, b );
    return eq;
} 

int relaxed_same_f( operation, a, b ) char * operation; float a; float b;
{
    float diff = a - b;
    float abs_diff = fabs( diff );
    bool eq = ( abs_diff <= ( 10 * TRIG_FLT_EPSILON ) );
    if ( !eq )
        printf( "operation %s: float %.20f is not the same as float %.20f\n", operation, a, b );
    return eq;
} 

int32_t factorial( n ) int32_t n;
{
    if ( 0 == n )
        return 1;

    return n * factorial( n - 1 );
} 

float my_pow( base, exp ) float base; int32_t exp;
{
    int32_t i;
    float fone = 1.0;
    float result = fone;

    if ( exp < 0 )
    {
        base = fone / base;
        exp = -exp;
    }

    for ( i = 0; i < exp; i++ )
        result *= base;
    return result;
} 

float my_sin( x ) float x;
{
    int32_t tmp, fbang;
    float fone = 1.0;
    float fmone = -fone;
    float sign = fone;
    int32_t i;
    float result = 0.0;
    float flimit = TRIG_FLT_EPSILON / 2.0;
    float ftmp = fone;

    for ( i = 1; i <= 10 && fabs( ftmp ) > flimit; i++ )
    {
        tmp = 2 * i - 1;
        fbang = factorial( tmp );

        ftmp = sign * my_pow( x, tmp ) / fbang;
        result += ftmp;
        sign *= fmone;
    }

    return result;
} 

float my_asin( x ) float x;
{
    int32_t n;
    float term, result, tmp;
    float fone = 1.0;
    float ftwo = 2.0;
    float fthree = 3.0;
    float flimit = 0.0000001;

    if ( fabs( x ) > fone )
        return result;

    term = x;
    result = x;
    for ( n = 0; fabs( term ) >= flimit; n++ )
    {
        tmp = ftwo * n + fone;
        term *= x * x * tmp / ( ftwo * n + ftwo ) * tmp / ( ftwo * n + fthree );
        result += term;
    }

    return result;
} 

float my_acos( x ) float x;
{
    float fzero = 0.0;
    float fone = 1.0;
    float ftwo = 2.0;
    float fmone = -1.0;
    float low = fzero;
    float high = M_PI;
    float mid, cos_mid;
    float flimit = atof( "1e-6" );

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

        if ( fabs( x - cos_mid ) <= flimit )
            return mid;

        if ( cos_mid > x )
            low = mid;
        else
            high = mid;
    }

    return fzero;
} 

float my_atan2f( y, x ) float y; float x;
{
    float fzero = 0.0;
    float ftwo = 2.0;
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
} 

float my_cotan( f ) float f;
{
    float fone = 1.0;
    return fone / tan( f );
}

void many_trigonometrics()
{
    float fresult, fback;
    float f = ( M_PI / -2.0 ) + 0x000001; 
    float fincr = 0.071;
    float ftwo = 2.0;
    float flimit = M_PI / ftwo;
    float f_cos, f_sin, f_cot, fake_cot;

    while ( f < flimit )
    {
        fresult = tan( f );
        fback = atan( fresult );
        check_same_f( "tan", f, fback );

        fresult = sin( f );
        fback = my_asin( fresult );
        if ( !check_same_f( "sin", f, fback ) )
            printf( "  sin result: %f\n", fresult );

        fresult = my_sin( f );
        fback = my_asin( fresult );
        if ( !check_same_f( "my_sin", f, fback ) )
            printf( "  my_sin result: %f\n", fresult );

        f_cos = cos( f );
        f_sin = sin( f );
        fake_cot = f_cos / f_sin;
        f_cot = my_cotan( f );
        relaxed_same_f( "my_cotan", fake_cot, f_cot );

        f += fincr;
    }
} 

float my_sqrt( num ) float num;
{
    float fone = 1.0;
    float ftwo = 2.0;
    float x = num;
    float y = fone;
    float e = 0.00002;

    while ( ( x - y ) > e )
    {
        x = ( x + y ) / ftwo;
        y = num / x;
    }
    return x;
} 

float my_frexp( x, exp ) float x; int * exp;
{
    float fzero = 0.0;
    float fhalf = 0.5;
    float fone = 1.0;
    float ftwo = 2.0;
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
} 

float round( f ) float f;
{
    if ( f >= 0 )
        return (float) (int32_t) ( f + 0.5 );

    return (float) (int32_t) ( f + -0.5 );
} 

int fl_cl_test()
{
    float cur = -1.75;
    float inc = 0.25;
    float f, c, r;
    int32_t i, x, ifloor, iceil, iround;

    printf( "  value         floor                 ceil                  round\n" );
    printf( "  ---------     -----------------     -----------------     -----------------\n" );

    for ( i = 0; i < 15; i++ )
    {
        f = floor( cur );
        ifloor = (int32_t) f;
        c = ceil( cur );
        iceil = (int32_t) c;
        r = round( cur );
        iround = (int32_t) r;
        printf( "  %9f  |  %9f = %5ld  |  %9f = %5ld  |  %9f = %5ld\n", cur, f, ifloor, c, iceil, r, iround );
        cur += inc;
    }

    return 0;
} 

int modftest()
{
    float val = 3.14159;
    float fracpart;
    double intpart;

    fracpart = modf( val, & intpart );
    printf( "val %f, intpart %lf, fracpart %f\n", val, intpart, fracpart );
    return 0;
} 

int ldexptest()
{
    float x = 1.5;
    int power = 5;
    float result = ldexp( x, power );
    printf( "ldexp( %f, %d ) = %f\n", x, power, result );

    return 0;
} 

int ftoatest()
{
    float val = 3.14159;
    char buf[ 20 ], buf2[ 20 ];
    int precision = 8;

    ftoa( val, buf, precision, 'f' );
    printf( "ftoa result:     '%s'\n", buf );

    floattoa( buf, val, precision );
    printf( "floattoa result: '%s'\n", buf );

    return 0;
} 

int main()
{
    char ac[ 100 ];
    int exponent;
    float f1, f2, fm1, fr, fd, fs;
    float pi, radians, s, c, t, f, at, mantissa, b, fpointone, fthree, fonehundred, fonepoint38, a;
    float fone = 1.0;

    TRIG_FLT_EPSILON = 0.0005;
    M_PI = 3.14159265;

    floattoa( ac, -1.234567, 8 );
    printf( "float converted by floattoa: %s\n", ac );
    floattoa( ac, 1.234567, 8 );
    printf( "float converted by floattoa: %s\n", ac );
    floattoa( ac, 34.567, 8 );
    printf( "float converted by floattoa: %s\n", ac );

    printf( "float from printf: %f\n", 45.678 );

    f1 = 1.0;
    f2 = 20.2;
    fm1 = -1.342;
    fr = f2 * fm1;
    fd = 1000.0 / 3.0;
    fs = sqrt( fd );

    printf( "division result: %f, square root %f\n", fd, fs );

    floattoa( ac, fr, 6 );
    printf( "float converted with floattoa: %s\n", ac );

    printf( "result of 20.2 * -1.342: %f\n", fr );

    pi = M_PI;
    radians = pi / 180.0 * 30.0;
    printf( "pi in radians: %f\n", radians );

    s = sin( radians );
    printf( "sin of 30 degress is %f\n", s );

    s = my_sin( radians );
    printf( "my_sin of 30 degress is %f\n", s );

    s = sinh( 0.5 );
    printf( "sinh of 0.5 degress is %f\n", s );

    c = cos( radians );
    printf( "cos of 30 degrees is %f\n", c );

    c = cosh( 0.5 );
    printf( "cosh of 0.5 degrees is %f\n", c );

    t = tan( radians );
    printf( "tan of 30 degrees is %f\n", t );

    f = 1.0;
    at = atan( f );
    printf( "atan of %f is %f\n", f, at );

    at = atan2( 0.3, 0.2 );
    printf( "atan2 of 0.3, 0.2 is %f\n", at );

    c = acos( 0.3 );
    printf( "acos of 0.3 is %f\n", c );

    c = my_acos( 0.3 );
    printf( "my_acos of 0.3 is %f\n", c );

    s = asin( 0.3 );
    printf( "asin of 0.3 is %f\n", s );

    s = my_asin( 0.3 );
    printf( "my_asin of 0.3 is %f\n", s );

    f = tanh( 2.2 );
    printf( "tanh of 2.2 is %f\n", f );

    f = log( 0.3 );
    printf( "log of 0.3: %f\n", f );

    f = log10( 300.0 );
    printf( "log10 of 300: %f\n", f );

    mantissa = my_frexp( pi, &exponent );
    printf( "pi has mantissa: %f, exponent %d\n", mantissa, exponent );

    fl_cl_test();

    modftest();

    ldexptest();

    ftoatest();

    b = 2.7;
    fpointone = 0.1;
    fthree = 3.0;
    for ( a = 2.0; a < fthree; a += fpointone )
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

    fonehundred = 100.0;
    fonepoint38 = 1.38;
    for ( f = fone; f < fonehundred; f += fonepoint38 )
        check_same_f( "square root float", my_sqrt( f ), sqrt( f ) );

    printf( "test tf completed with great success\n" );
    exit( 0 );
} 

