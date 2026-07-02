#include <stdio.h>

int main( argc, argv ) int argc; char *argv[];
{
    printf( "printf says hello\n" );
    fprintf( stdout, "fprintf( stdout ) says hello\n" );
    fprintf( stderr, "fprintf( stderr ) says hello\n" );
}
