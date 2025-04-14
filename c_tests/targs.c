#include <stdio.h>

int main( int argc, char * argv[], char * penv[] )
{
    printf( "argc: %d\n", argc );

    for ( int i = 0; i < argc; i++ )
        printf( "argv[ %d ]: '%s'\n", i, argv[ i ] );

    if ( 0 != penv )
        for ( int i = 0; penv[ i ]; i++ )
           printf( "penv[ %d ]: '%s'\n", i, penv[ i ] );

    return 0;
} // main

