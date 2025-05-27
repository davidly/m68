#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#ifdef WATCOM
#include <malloc.h>
#include <process.h>
#include <string.h>
#include <io.h>
#endif

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define TEST_FILE "fopentst.txt"

#ifdef _MSC
#define DOSTIME
#endif

void show_local_time( time_t tt, const char * ptz )
{
    if ( 0 != ptz )
    {
        char * penv = (char *) malloc( strlen( ptz ) + 4 );
        strcpy( penv, "TZ=" );
        strcat( penv, ptz );
        putenv( penv );
    }

    const char * timezoneval = getenv( "TZ" );
    if ( 0 == timezoneval )
        timezoneval = "(null)";

    struct tm * t = localtime( &tt );

    printf( "tz: '%s', year: %d, month %d, day %d, hour %d, min %d, sec %d\n",
            timezoneval, t->tm_year + 1900, 1 + t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec );
} //show_local_time

#ifdef DOSTIME
long portable_filelen( fp ) FILE * fp;
#else
long portable_filelen( FILE * fp )
#endif
{
    long len = 0;
    long current = ftell( fp );
    fseek( fp, len, SEEK_END );
    len = ftell( fp );
    fseek( fp, current, SEEK_SET );

    printf( "len in pfl: %ld\n", len );
    return len;
} 

#ifdef DOSTIME
int main( argc, argv ) int argc; char *argv[];
#else
int main( int argc, char * argv[] )
#endif
{
    FILE * fp;
    int i;
    long l;

#ifdef O_CREAT
    printf( "O_CREAT: %#x\n", O_CREAT );
#endif
#ifdef O_TRUNC    
    printf( "O_TRUNC: %#x\n", O_TRUNC );
#endif
#ifdef O_ASYNC 
    printf( "O_ASYNC: %#x\n", O_ASYNC );
#endif
#ifdef O_BINARY
    printf( "O_BINARY: %#x\n", O_BINARY );
#endif
#ifdef O_TEXT
    printf( "O_TEXT: %#x\n", O_TEXT );
#endif
#ifdef O_TEMPORARY
    printf( "O_TEMPORARY: %#x\n", O_TEMPORARY );
#endif
#ifdef O_FSYNC
    printf( "O_FSYNC: %#x\n", O_FSYNC );
#endif
#ifdef O_RANDOM
    printf( "O_RANDOM: %#x\n", O_RANDOM );
#endif
#ifdef O_SYNC
    printf( "O_SYNC: %#x\n", O_SYNC );
#endif
#ifdef O_SEQUENTIAL
    printf( "O_SEQUENTIAL: %#x\n", O_SEQUENTIAL );
#endif
#ifdef O_NOINHERIT
    printf( "O_NOINHERIT: %#x\n", O_NOINHERIT );
#endif
#ifdef O_RDONLY
    printf( "O_RDONLY: %#x\n", O_RDONLY );
#endif
#ifdef O_WRONLY
    printf( "O_WRONLY: %#x\n", O_WRONLY );
#endif
#ifdef O_RDWR
    printf( "O_RDWR: %#x\n", O_RDWR );
#endif
#ifdef O_APPEND
    printf( "O_APPEND: %#x\n", O_APPEND );
#endif
#ifdef O_EXCL
    printf( "O_EXCL: %#x\n", O_EXCL );
#endif
#ifdef O_EXLOCK
    printf( "O_EXCLOCK: %#x\n", O_EXCLOCK );
#endif
#ifdef O_SHLOCK
    printf( "O_SHCLOCK: %#x\n", O_SHCLOCK );
#endif
#ifdef O_DIRECTORY
    printf( "O_DIRECTORY: %#x\n", O_DIRECTORY );
#endif
#ifdef O_DIRECT
    printf( "O_DIRECT: %#x\n", O_DIRECT );
#endif

#ifdef DOSTIME
    unlink( TEST_FILE ); /* in case it's still there. */
#else
    remove( TEST_FILE );
#endif

    fp = fopen( TEST_FILE, "w" );
    if ( !fp )
    {
        printf( "can't create test file, error %d\n", errno );
        exit( 1 );
    }

    for ( i = 0; i < 10; i++ )
        fprintf( fp, "line %d\n", i );

    l = portable_filelen( fp );
    printf( "length of file before initial close: %d\n", (int) l );

    fclose( fp );

    struct stat statbuf;
    memset( &statbuf, 0, sizeof( statbuf ) );
    stat( TEST_FILE, &statbuf );
    printf( "length from stat: %d\n", (int) statbuf.st_size );

    int fd = open( TEST_FILE, O_RDONLY );
    memset( &statbuf, 0, sizeof( statbuf ) );
    fstat( fd, &statbuf );
    printf( "length from fstat: %d\n", (int) statbuf.st_size );
    close( fd );

    show_local_time( statbuf.st_mtime, "PST+8" );

    fp = fopen( TEST_FILE, "w+t" );
    if ( !fp )
    {
        printf( "can't create test file, error %d\n", errno );
        exit( 1 );
    }

    l = portable_filelen( fp );
    if ( 0 != l )
    {
        printf( "expected 0 length; length of file after recreation: %d\n", (int) l );
        exit( 1 );
    }

    fprintf( fp, "new line 0\n" );
    fclose( fp );

    printf( "exiting fopentst with great success\n" );
    return 0;
} /*main*/

