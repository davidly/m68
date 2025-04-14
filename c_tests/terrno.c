#include <stdio.h>
#include <cerrno>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

int main( int argc, char * argv[] )
{
    FILE * fp = fopen( "notthere.txt", "r" );
    if ( fp )
    {
        printf( "notthere.txt was opened, unexpectedly. errno: %d\n", errno );
        exit( 1 );
    }

    printf( "errno opening a file for read that doesn't exist: %d (2 file not found expected)\n", errno );

#if 0 // this causes a watson dump
    int result = write( 55, "hello", 5 );
    if ( -1 != result )
    {
        printf( "write to invalid file descriptor succeeded result %d, errno %d\n", result, errno );
        exit( 1 );
    }

    printf( "errno doing write to an invalid file descriptor: %d\n", errno );
#endif

    int result = write( 0, "hello", 5 );
    if ( -1 != result )
    {
        printf( "write to stdin file descriptor succeeded result %d, errno %d\n", result, errno );
        exit( 1 );
    }

    printf( "errno doing write to stdin file descriptor: %d. (13 permission denied expected)\n", errno );

    fp = fopen( "notthere.txt", "zzz" );
    if ( fp )
    {
        printf( "notthere.txt with invalid open flags was opened, unexpectedly. errno: %d\n", errno );
        exit( 1 );
    }

    printf( "errno opening a file for read with invalid open flags: %d (22 invalid argument expected)\n", errno );

    printf( "errno test completed with great success\n" );
    return 0;
} //main
