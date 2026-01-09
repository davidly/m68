#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

long lseek();

#define FileA "trenameA.txt"
#define FileB "trenameB.txt"

long portable_filelen( fp ) FILE * fp;
{
    int result;
    long len;
    long offset;
    long current = ftell( fp );
    printf( "current offset: %ld\n", current );
    offset = 0;
    result = fseek( fp, offset, SEEK_END );
    printf( "result of fseek: %d\n", result );
    len = ftell( fp );
    printf( "file length from ftell: %ld\n", len );
    fseek( fp, current, SEEK_SET );
    return len;
}

long fl_path( path ) char * path;
{
    long len;
    FILE * fp;
    fp = fopen( path, "r" );
    if ( 0 == fp )
        return -1;

    len = portable_filelen( fp );
    fclose( fp );
    return len;
}

void error( err ) char * err;
{
    if ( err )
        printf( "error: %s ==== errno: %d\n", err, errno );
    exit( 1 );
} /*error*/

int main( argc, argv ) int argc; char * argv[];
{
    long len;
    FILE * fA;
    static char data[ 1024 ];
    int result;

    unlink( FileA );
    unlink( FileB );

    len = fl_path( FileA );
    if ( -1 != len )
        error( "file A shouldn't exist" );

    len = fl_path( FileB );
    if ( -1 != len )
        error( "file B shouldn't exist" );

    {
        FILE * fA = fopen( FileA, "w" );
        if ( 0 == fA )
            error( "can't create file A" );

        memset( & data, 3, sizeof( data ) );
        result = fwrite( &data, 1, sizeof( data ), fA );
        if ( result != sizeof( data ) )
        {
            printf( "result: %ld\n", result );
            error( "can't write data to file A" );
        }

        result = fclose( fA );
        if ( 0 != result )
            error( "can't close file A" );
    }

    result = rename( FileA, FileB );
    if ( 0 != result )
        error( "rename A to B failed" );

    len = fl_path( FileA );
    if ( -1 != len )
        error( "file A shouldn't exist" );

    len = fl_path( FileB );
    if ( -1 == len )
        error( "file B should exist but apparently doesn't" );

    result = unlink( FileB );
    if ( 0 != result )
        error( "can't unlink file B" );

    printf( "trename completed with great success\n" );
    return 0;
} /*main*/


