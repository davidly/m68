#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>

int main()
{
    double pi = 3.14159265358979323846264338327952884197169399375105820974944592307;
    printf( "double: %f\n", pi );

    char ac[100];
    sprintf( ac, "sprintf double %.20f\n", pi );
    printf( ac );

    printf( "double from printf: %.20f\n", pi );

    float f = 1.2020569;
    printf( "float from printf: %f\n", f );

    double r = -f * pi;
    printf( "double from printf r: %lf\n", r );

    double sq = sqrt( pi );
    printf( "sqrt of pi: %lf\n", sq );

    double radians = ( 30.0 * pi ) / 180.0;
    printf( "pi in radians: %lf\n", radians );

    double s = sin( radians );
    printf( "sin of 30 degress is %lf\n", s );

    double c = cos( radians );
    printf( "cos of 30 degrees is %lf\n", c );

    double t = tan( radians );
    printf( "tan of 30 degrees is %lf\n", t );

    double d = atof( "1.0" );
    double at = atan( d );
    printf( "atan of %lf is %lf\n", d, at );

    at = atan2( 0.3, 0.2 );
    printf( "atan2 of 0.3, 0.2 is %lf\n", at );

    c = acos( 0.3 );
    printf( "acos of 0.3 is %lf\n", c );

    s = asin( 0.3 );
    printf( "asin of 0.3 is %lf\n", s );

    d = tanh( 2.2 );
    printf( "tanh of 2.2 is %lf\n", s );
    
    d = log( 0.3 );
    printf( "log of 0.3: %lf\n", d );

    d = log10( 300.0 );
    printf( "log10 of 300: %lf\n", d );
    
    double b = 0.2;
    for ( double a = -0.5; a < 0.5; a += 0.1 )
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

    int exponent;
    double mantissa = frexp( pi, &exponent );
    printf( "pi has mantissa: %lf, exponent %d\n", mantissa, exponent );

    int loops = 1000;
    r = 1.0;
    for ( int i = 0; i < loops; i++ )
        r *= 1.14157222;

    t = r;
    for ( int i = 0; i < loops; i++ )
        r /= 1.14157222;

    printf( "r should be 1.0: %lf\n", r );
    printf( "  r high point %lf\n", t );
    
    printf( "test doubles completed with great success\n" );
    return 0;
} //main

