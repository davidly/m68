#include <stdio.h>

int main( int argc, char * argv[] )
{
    //printf( "%2d\n", 6 );
    printf( "hi %d\n", -17 );
    printf( "hi %zd\n", -17 );
    printf( "float %lf\n", 6.789 );
    printf( "float %lf\n", 6767676767.7890123456 );
    //printf( "hi %d\n", 6493 );

    fflush( stdout );
    return 0;
}
