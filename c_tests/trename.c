#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#define FileA "trenameA.txt"
#define FileB "trenameB.txt"

long portable_filelen( FILE * fp )
{
    long current = ftell( fp );
    long offset = 0;
    int result = fseek( fp, offset, SEEK_END );
    long len = ftell( fp );
    fseek( fp, current, SEEK_SET );
    return len;
} //portable_filelen

long portable_filelen( const char * path )
{
    FILE * fp = fopen( path, "r" );
    if ( 0 == fp )
        return -1;

    long len = portable_filelen( fp );
    fclose( fp );
    return len;
} //portable_filelen

void error( const char * err )
{
    if ( err )
        printf( "error: %s ==== errno: %d\n", err, errno );
    exit( 1 );
} //error

int main( int argc, char * argv[] )
{
    remove( FileA );
    remove( FileB );

    long len = portable_filelen( FileA );
    if ( -1 != len )
        error( "file A shouldn't exist at point 1" );

    len = portable_filelen( FileB );
    if ( -1 != len )
        error( "file B shouldn't exist at point 1" );

    {
        FILE * fA = fopen( FileA, "w" );
        if ( 0 == fA )
            error( "can't create file A" );
    
        static uint8_t data[ 1024 ];
        memset( &data, 3, sizeof( data ) );
        int result = fwrite( &data, 1, sizeof( data ), fA );
        if ( result != sizeof( data ) )
        {
            printf( "result: %ld\n", result );
            error( "can't write data to file A" );
        }
    
        result = fclose( fA );
        if ( 0 != result )
            error( "can't close file A" );
    }

    int result = rename( FileA, FileB );
    if ( 0 != result )
        error( "rename A to B failed" );

    len = portable_filelen( FileA );
    if ( -1 != len )
        error( "file A shouldn't exist after rename" );

    len = portable_filelen( FileB );
    if ( -1 == len )
        error( "file B should exist but apparently doesn't" );

    // linux (unlike windows or cp/m) supports renaming a file over an existing file!\n" );
    char acdir[ 100 ];
    char * pcwd = getcwd( acdir, sizeof( acdir ) );
    if ( 0 == pcwd )
        error( "getcwd failed" );

    if ( strcmp( acdir, "." ) ) // test this if we're not running on CP/M 68K, which has no directories 
    {
        FILE * fA = fopen( FileA, "w" );
        if ( 0 == fA )
            error( "can't create file A a second time" );

        fprintf( fA, "fileA data I DONT CARE bdc\n" );

        result = fclose( fA );
        if ( 0 != result )
            error( "can't close file A a second time" );
    
        result = rename( FileA, FileB );
        if ( 0 != result )
            error( "rename A to B a second time failed" );
    }

    result = remove( FileB );
    if ( 0 != result )
        error( "can't remove file B" );

    printf( "trename completed with great success\n" );
    return 0;
} //main


