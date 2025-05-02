#include <stdio.h>

#ifdef WATCOM
#include <malloc.h>
#include <process.h>
#include <string.h>
#include <io.h>
#endif

typedef char uint8_t;
typedef char int8_t;
typedef short uint16_t;
typedef short int16_t;
typedef long uint32_t;
typedef long int32_t;
typedef int bool;

#define false 0
#define true 1
#define INT_MAX 32767

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

char acbuf[ 128 ];
char acbuf512[ 512 ];

FILE * fopen();
int errno;

char * memset( p, v, c ) char * p; int v; int c;
{
    int i;
    char * orig;

    orig = p;
    for ( i = 0; i < c; i++ )
        *p++ = (char) v;
    return orig;
}

/* this does not work on Aztec -- fseek to SEEK_END closes the file and doesn't re-open it! */

long portable_filelen( fp ) FILE * fp;
{
    int result;
    long len;
    long offset;
    long current;
    current = ftell( fp );
    printf( "current offset: %ld\n", current );
    offset = 0;
    result = fseek( fp, offset, SEEK_END );
    printf( "result of fseek: %d\n", result );
    len = ftell( fp );
    printf( "file length from ftell: %ld\n", len );
    fseek( fp, current, SEEK_SET );
    return len;
} 

int read_and_validate( offset, chunkLen, fp ) long offset; int chunkLen; FILE * fp;
{
    int result;

    result = fread( acbuf512, 1, chunkLen, fp );
    printf( "result of read at offset %ld: %d\n", offset, result );
    if ( 0 == result )
        printf( "  errno (0 if at eof): %d\n", errno );
    else
    {
        if ( 512 == offset )
        {
            if ( acbuf512[ 0 ] != 'k' )
                printf( "data at fixed offset 512 isn't a k\n" );
            if ( acbuf512[ 127 ] != 'k' )
                printf( "data at fixed offset 512 + 127 isn't a k\n" );
            if ( acbuf512[ 128 ] != 0 )
                printf( "data at fixed offset 512 + 128 isn't a 0\n" );
        }
        else if ( 8192 == offset )
        {
            if ( acbuf512[ 0 ] != 'j' )
                printf( "data at fixed offset 8192 isn't a j\n" );
            if ( acbuf512[ 127 ] != 'j' )
                printf( "didn't find a 'j' at the end of the file, it's %x\n", acbuf512[ 127 ] );
        }
        else
        {
            if ( 0 != acbuf512[ 0 ] )
                printf( "data at offset %d isn't a 0\n", offset );
            if ( 0 != acbuf512[ 511 ] )
                printf( "data at offset %d isn't a 0\n", offset + 511 );
        }
    }
} /*read_and_validate*/

int main( argc, argv ) int argc; char *argv[];
{
    char * pcFile;
    FILE * fp;
    long len, offset;
    int chunkLen;
    int result;
    pcFile = "fileops.dat";

    unlink( pcFile );

    fp = fopen( pcFile, "wb+" );
    printf( "fp: %lx\n", fp );
    if ( 0 == fp )
    {
        printf( "unable to open file, errno %d\n", errno );
        exit( 1 );
    }

    len = portable_filelen( fp );
    printf( "empty file length: %ld\n", len );

    offset = 8192;
    result = fseek( fp, offset, SEEK_SET );
    printf( "result of fseek: %d\n", result );
    if ( -1 == result )
        printf( "errno after fseek: %d\n", errno );

    memset( acbuf, 'j', sizeof( acbuf ) );

    result = fwrite( acbuf, sizeof( acbuf ), 1, fp );
    printf( "result of fwrite (should be 1): %d\n", result );
    if ( 1 != result )
        printf( "errno after fwrite: %d\n", errno );

    len = portable_filelen( fp );
    printf( "8192 + 128 = 8320 file length from portable_filelen: %ld\n", len );

    offset = 512;
    result = fseek( fp, offset, SEEK_SET );
    printf( "result of fseek to middle of file: %d\n", result );
    if ( -1 == result )
        printf( "errno after fseek to middle of file: %d\n", errno );

    memset( acbuf, 'k', sizeof( acbuf ) );
    result = fwrite( acbuf, sizeof( acbuf), 1, fp );
    printf( "result of fwrite to middle of file (should be 1): %d\n", result );
    if ( 1 != result )
        printf( "errno after fwrite middle of file: %d\n", errno );

    fflush( fp );
    fclose( fp );

    fp = fopen( pcFile, "rb+" );
    printf( "fp: %lx\n", fp );
    if ( 0 == fp )
    {
        printf( "unable to open file a second time, errno %d\n", errno );
        exit( 1 );
    }

    len = portable_filelen( fp );
    printf( "8192 + 128 = 8320 file length from cpm_filelen: %ld\n", len );

    memset( acbuf512, 'd', sizeof( acbuf512 ) );
    chunkLen = 512;
    for ( offset = 0; offset < 8320; offset += chunkLen )
        read_and_validate( offset, chunkLen, fp );

    /* now read in blocks from the end of the file to the start using fseek() */

    printf( "testing backwards read\n" );

    memset( acbuf512, 'e', sizeof( acbuf512 ) );
    for ( offset = 8192; offset >= 0; offset -= chunkLen )
    {
        result = fseek( fp, offset, SEEK_SET );
        read_and_validate( offset, chunkLen, fp );
    }

    fclose( fp );
    return 0;
} /*main*/
