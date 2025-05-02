#include <stdio.h>

static char ac[ 20 ];

int main( argc, argv ) int argc; char * argv[];
{
    float f;
    long prec;
    BYTE c;

    ac[ 0 ] = 0;
    f = 3.14;
    printf( "pi: %f\n", f );

    prec = 4;
    c = 'f';
    ftoa( f, ac, prec, c );

    puts( ac );
    return 0;
}

