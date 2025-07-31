#include <stdio.h>
#include <errno.h>

FILE * fopen();
extern int errno;

#define DOSTIME

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define TEST_FILE "fopentst.txt"

#ifdef _MSC
#define DOSTIME
#endif

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
    printf( "O_EXLOCK: %#x\n", O_EXLOCK );
#endif
#ifdef O_SHLOCK
    printf( "O_SHLOCK: %#x\n", O_SHLOCK );
#endif



#ifdef DOSTIME
    unlink( TEST_FILE ); /* in case it's still there. */
#else
    remove( TEST_FILE );
#endif

    fp = fopen( TEST_FILE, "w" );
    if ( 0 == fp )
    {
        printf( "can't create test file, error %d\n", errno );
        exit( 1 );
    }

    for ( i = 0; i < 10; i++ )
        fprintf( fp, "line %d\n", i );

    l = portable_filelen( fp );
    printf( "length of file before initial close: %d\n", (int) l );

    fclose( fp );

    fp = fopen( TEST_FILE, "w+t" );
    if ( 0 == fp )
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

