/* validate that opening a file with fopen( X, "w" ) truncates the file */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

extern int errno;

long portable_filelen( FILE * fp )
{
    int result;
    long len;
    long offset;
    long current = ftell( fp );
    //printf( "current offset: %ld\n", current );
    offset = 0;
    result = fseek( fp, offset, SEEK_END );
    //printf( "result of fseek: %d\n", result );
    len = ftell( fp );
    //printf( "file length from ftell: %ld\n", len );
    fseek( fp, current, SEEK_SET );
    return len;
} 

int main( int argc, char * argv[] )
{
    FILE * fp = fopen( "tfo.txt", "w" );
    if ( !fp )
    {
        printf( "file open failed, error %d\n", errno );
        exit( 1 );
    }

    printf( "filelen: %lu\n", portable_filelen( fp ) );
    fprintf( fp, "the contents of the text file!\n" );
    printf( "filelen: %lu\n", portable_filelen( fp ) );
    fclose( fp );

    fp = fopen( "tfo.txt", "w" );
    if ( !fp )
    {
        printf( "file open the second time failed, error %d\n", errno );
        exit( 1 );
    }

    long len = portable_filelen( fp );
    if ( 0 != len )
    if ( !fp )
    {
        printf( "opening the file again for write didn't truncate the file!\n" );
        exit( 1 );
    }

    fclose( fp );
    remove( "tfo.txt" );
    
    printf( "tfo completed with great success\n" );
    return 0;
}