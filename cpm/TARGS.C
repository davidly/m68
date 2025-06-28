#include <stdio.h>

int main( argc, argv, penv ) int argc; char * argv[]; char * penv[];
{
    int i;

    printf( "argc: %d, argv %lx, penv %lx\n", argc, argv, penv );

    for ( i = 0; i < argc; i++ )
        printf( "argv[ %d ]: '%s'\n", i, argv[ i ] );

    if ( 0 != penv )
        for ( i = 0; penv[ i ]; i++ )
           printf( "penv[ %d ]: '%s'\n", i, penv[ i ] );

    return 0;
} // main

