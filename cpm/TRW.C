#include <stdio.h>

long lseek();

int show_error( str ) char * str;
{
    printf( "error: %s\n", str );
    exit( 1 );
}

#define TRW_FILE "trw.dat"
#define BUF_ELEMENTS 32
static short buf[ BUF_ELEMENTS ];
#define BUF_SIZE ( sizeof( buf ) )

int main()
{
    int i, j, result, fd, buf_bytes;
    long int seek_offset, file_offset;

    unlink( TRW_FILE );
    fd = creatb( TRW_FILE, 2 );
    if ( -1 == fd )
        show_error( "unable to create data file" );

    result = close( fd );
    if ( 0 != result )
        show_error( "close of file descriptor at point A failed\n" );

    fd = openb( TRW_FILE, 2 );
    if ( -1 == fd )
        show_error( "unable to open data file for write" );

    buf_bytes = sizeof( buf );

    for ( i = 0; i < 4096; i++ )
    {
        for ( j = 0; j < BUF_ELEMENTS; j++ )
            buf[ j ] = i;

        result = write( fd, (char *) buf, buf_bytes );
        if ( buf_bytes != result )
        {
            printf( "write failed in first loop. i %d, result %x, buf_bytes %x\n", i, result, buf_bytes );
            show_error( "unable to write to file in first loop" );
        }
    }

    result = close( fd );
    if ( 0 != result )
        show_error( "close of file descriptor at point B failed\n" );

    fd = openb( TRW_FILE, 0 );
    if ( -1 == fd )
        show_error( "unable to open data file read only" );

    for ( i = 0; i < 4096; i++ )
    {
        result = read( fd, (char *) buf, buf_bytes );
        if ( buf_bytes != result )
        {
            printf( "result: %d, i %d\n", result, i );
            show_error( "unable to read from file at point A" );
        }

        for ( j = 0; j < BUF_ELEMENTS; j++ )
        {
            if ( buf[ j ] != i )
            {
                printf( "j %d, buf[j] %04x, i %d\n", j, buf[j], i );
                show_error( "data read from file isn't what was expected at point A" );
            }
        }
    }

    result = close( fd );
    if ( 0 != result )
        show_error( "close of file descriptor at point C failed\n" );

    fd = openb( TRW_FILE, 2 );
    if ( -1 == fd )
        show_error( "unable to open data file read/write" );

    for ( i = 0; i < 4096; i++ )
    {
        if ( 0 == ( i % 8 ) )
        {
            seek_offset = (long int) i * BUF_SIZE;
            file_offset = lseek( fd, seek_offset, 0 );
            if ( file_offset != seek_offset )
            {
                printf( "file_offset %ld, seek_offset %ld\n", file_offset, seek_offset );
                show_error( "lseek location not as expected" );
            }

            for ( j = 0; j < BUF_ELEMENTS; j++ )
                buf[ j ] = i + 0x4000;

            result = write( fd, (char *) buf, buf_bytes );
            if ( buf_bytes != result )
                show_error( "unable to write to file after lseek" );
        }
    }

    result = close( fd );
    if ( 0 != result )
        show_error( "close of file descriptor at point D failed\n" );

    fd = openb( TRW_FILE, 0 );
    if ( -1 == fd )
        show_error( "unable to open data file read only" );

    for ( i = 4095; i >= 0; i-- )
    {
        seek_offset = (long int) i * BUF_SIZE;
        file_offset = lseek( fd, seek_offset, 0 );
        if ( file_offset != seek_offset )
        {
            printf( "file_offset %ld, seek_offset %ld\n", file_offset, seek_offset );
            show_error( "lseek location not as expected" );
        }

        result = read( fd, (char *) buf, buf_bytes );
        if ( buf_bytes != result )
            show_error( "unable to read from file after lseek" );

        for ( j = 0; j < BUF_ELEMENTS; j++ )
        {
            if ( 0 == ( i % 8 ) )
            {
                if ( buf[ j ] != i + 0x4000 )
                    show_error( "data read from file isn't what was expected at point B" );
            }
            else
            {
                if ( buf[ j ] != i )
                    show_error( "data read from file isn't what was expected at point C" );
            }
        }
    }

    result = close( fd );
    if ( 0 != result )
        show_error( "close of file descriptor at point E failed\n" );

    result = unlink( TRW_FILE );
    if ( 0 != result )
        show_error( "can't unlink test file" );

    printf( "trw completed with great success\n" );
    return 0;
}

