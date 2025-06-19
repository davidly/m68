// This test app is heavily-tailored to the DR CP/M 68K v1.3 C compiler and runtime.
// Note that the v1.2 libf.a must be used because the v1.3 version has many bugs.
// But add the v1.3 version afterwards because it has some working functions not implemented in v1.2.
// Like this: m68 lo68.68k -r -u__optoff -o %1.68k s.o %1.o clib libf12.a libf.a
// Floating point constants don't work with this compiler, so atof() is used throughout
// note: I validated the broken behaviors on a physical 68008 machine.
// functions linked from v1.2's libf12.a:
//    _atof
//    _ceil
//    _cos
//    _fabs
//    _floor
//    _ftoa
//    _log
//    _sin
//    _sqrt
// helper functions linked from v1.2's libf12.a:
//    _fpadd
//    _fpcmp
//    _fpdiv
//    _fpftol
//    _fpltof
//    _fpmult
//    _fpneg
//    _fpsub
// functions linked from v1.3's libf.a:
//    _atan
//    _atan2
//    _cosh
//    _log10
//    _modf
//    _sinh
//    _tan
//    _tanh
// functions that don't exist and need private versions:
//    _acos
//    _asin
// functions that don't work correctly and need private versions:
//    _pow      returns positive results for calls like pow( -2, 3 )
//    _frexp    incorrect results for calls like frexp( 3.14159, & exponent )
//              returns mantissa:       1.570801, exponent 1
//              should return mantissa: 0.785398, exponent 2

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

// this old compiler doesn't support the syntax that converts x into a string. so flt is unused.

#define flt( x ) ( atof( #x ) )

// these operate on floats, not doubles, per old naming conventions.
// some don't exist (acos, asin) so alternative my_* versions are below.
// some don't work correctly (pow, frexp) so alternative my_* versions are below.
// prototypes must exist because the default return type is a 2-byte int.

float acos();
float asin();
float atan();
float atan2();
float atof();
float ceil();
float cos();
float cosh();
float fabs();
float floor();
float frexp();
char * ftoa();
float ldexp();
float log();
float log10();
float modf();
float pow();
float sin();
float sinh();
float sqrt();
float tan();
float tanh();

float TRIG_FLT_EPSILON;
float M_PI;

char * floattoa( buffer, f, precision ) char *buffer; float f; int precision;
{
    int32_t whole;
    float fraction;
    char * pbuf = buffer;
    float ften = atof( "10" );
    float fzero = atof( "0" );
    float fmone = atof( "-1" );

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
} //floattoa

int check_same_f( operation, a, b ) char * operation; float a; float b;
{
    float diff = a - b;
    float abs_diff = fabs( diff );
    bool eq = ( abs_diff <= TRIG_FLT_EPSILON );
    if ( !eq )
        printf( "operation %s: float %.20f is not the same as float %.20f\n", operation, a, b );
    return eq;
} //check_same_f

int relaxed_same_f( operation, a, b ) char * operation; float a; float b;
{
    float diff = a - b;
    float abs_diff = fabs( diff );
    bool eq = ( abs_diff <= ( 10 * TRIG_FLT_EPSILON ) );
    if ( !eq )
        printf( "operation %s: float %.20f is not the same as float %.20f\n", operation, a, b );
    return eq;
} //relaxed_same_f

int32_t factorial( n ) int32_t n;
{
    if ( 0 == n )
        return 1;

    return n * factorial( n - 1 );
} //factorial

float my_pow( base, exp ) float base; int32_t exp;
{
    int32_t i;
    float fone = atof( "1.0" );
    float result = fone;

    if ( exp < 0 )
    {
        base = fone / base;
        exp = -exp;
    }

    for ( i = 0; i < exp; i++ )
        result *= base;
    return result;
} //my_pow

float my_sin( x ) float x;
{
    int32_t tmp, fbang;
    float fone = atof( "1.0" );
    float fmone = -fone;
    float sign = fone;
    int32_t i;
    float result = atof( "0.0" );
    float flimit = TRIG_FLT_EPSILON / atof( "2" );
    float ftmp = fone;

    for ( i = 1; i <= 10 && fabs( ftmp ) > flimit; i++ ) 
    {
        tmp = 2 * i - 1;
        fbang = factorial( tmp );

        // pow() in the C runtime produces incorrect results;
        // pow() returns a positive value when the first argument is negative and the second is an odd number.
        // note when using pow: its second argument is a float, not an int32_t.

        ftmp = sign * my_pow( x, tmp ) / fbang;
        result += ftmp;
        sign *= fmone;
    }

    return result;
} //my_sin

float my_asin( x ) float x;
{
    int32_t n;
    float term, result, tmp;
    float fone = atof( "1.0" );
    float ftwo = atof( "2.0" );
    float fthree = atof( "3.0" );
    float flimit = atof( ".0000001" );

    if ( fabs( x ) > fone )
        return result;

    term = x;
    result = x;
    for ( n = 0; fabs( term ) >= flimit; n++ )
    {
        tmp = ftwo * n + fone;
        term *= x * x * tmp / ( ftwo * n + ftwo ) * tmp / ( ftwo * n + fthree );
        result += term;
        // printf( "    n %ld, term %f, result %f\n", n, term, result );
    }

    //if ( n > 2000 )
    //    printf( "my_asin took %ld loops\n", n );

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

float my_cotan( f ) float f;
{
    float fone = atof( "1" );
    return fone / tan( f );
}

void many_trigonometrics()
{
    float fresult, fback;
    float f = ( M_PI / atof( "-2" ) ) + 0x000001; // want to be >= negative half pi.
    float fincr = atof( "0.071" );
    float ftwo = atof( "2" );
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
} //many_trignometrics

float my_sqrt( num ) float num;
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
} //my_sqrt

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

float round( f ) float f;
{
    if ( f >= 0 )
        return (float) (int32_t) ( f + atof( "0.5" ) );

    return (float) (int32_t) ( f + atof( "-0.5" ) );
} //round

int fl_cl_test()
{
    float cur = atof( "-1.75" );
    float inc = atof( "0.25" );
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
} //fl_cl_test

int modftest()
{
    float val = atof( "3.14159" );
    float fracpart, intpart;

    fracpart = modf( val, & intpart );
    printf( "val %f, intpart %f, fracpart %f\n", val, intpart, fracpart );
    return 0;
} //modftest

int ldexptest()
{
    float x = atof( "1.5" );
    int power = 5;
    float result = ldexp( x, power );
    printf( "ldexp( %f, %d ) = %f\n", x, power, result );

    return 0;
} //ldexptest

int ftoatest()
{
    float val = atof( "3.14159" );
    char buf[ 20 ], buf2[ 20 ];
    int precision = 8;

    // note that ftoa and floattoa arguments are different. Looking at disassembly I think ftoa doesn't use the last format argument.

    ftoa( val, buf, precision, 'f' );
    printf( "ftoa result:     '%s'\n", buf );

    floattoa( buf, val, precision );
    printf( "floattoa result: '%s'\n", buf );

    return 0;
} //ftoatest

int main()
{
    char ac[ 100 ];
    int exponent;
    float f1, f2, fm1, fr, fd, fs;
    float pi, radians, s, c, t, f, at, mantissa, b, fpointone, fthree, fonehundred, fonepoint38, a;
    float fone = atof( "1" );

    TRIG_FLT_EPSILON = atof( "0.0005" ); // atof( "0.00002" );  /* 0.00000011920928955078 would be better, but trig functions don't have that precision */
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

    s = my_sin( radians );
    printf( "my_sin of 30 degress is %f\n", s );
                                        
    s = sinh( atof( "0.5" ) );
    printf( "sinh of 0.5 degress is %f\n", s );

    c = cos( radians );
    printf( "cos of 30 degrees is %f\n", c );

    c = cosh( atof( "0.5" ) );
    printf( "cosh of 0.5 degrees is %f\n", c );

    t = tan( radians );
    printf( "tan of 30 degrees is %f\n", t );

    f = atof( "1.0" );
    at = atan( f );
    printf( "atan of %f is %f\n", f, at );

    at = atan2( atof( "0.3" ), atof( "0.2" ) );
    printf( "atan2 of 0.3, 0.2 is %f\n", at );

    // acos() doesn't exist in the C runtime
    c = my_acos( atof( "0.3" ) );
    printf( "acos of 0.3 is %f\n", c );

    // asin() doesn't exist in the C runtime
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

    fl_cl_test();

    modftest();

    ldexptest();

    ftoatest();

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
        check_same_f( "square root float", my_sqrt( f ), sqrt( f ) );

    printf( "test tf completed with great success\n" );
    exit( 0 );
} //main

