#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

int main( int argc, char * argv[] )
{
    const char * foldername = "tdir_testfolder";
    const char * filename = "tdir_testfile.txt";

    int result = mkdir( foldername, S_IRWXU | S_ISVTX );
    if ( 0 != result )
    {
        printf( "mkdir failed, error %d\n", errno );
        printf( "folder '%s' exists from a prior run; deleting it\n", foldername );
        fflush( stdout );
        result = rmdir( foldername );
        if ( 0 != result )
        {
            printf( "start of app cleanup: rmdir of folder %s failed, error %d\n", foldername, errno );
            exit( 1 );
        }

        int result = mkdir( foldername, S_IRWXU | S_ISVTX );
        if ( 0 != result )
        {
            printf( "creation of folder %s failed, error %d\n", foldername, errno );
            exit( 1 );
        }
    }

    result = chdir( foldername );
    if ( 0 != result )
    {
        printf( "cd into the test folder %s failed, error %d\n", foldername, errno );
        exit( 1 );
    }

    FILE * fp = fopen( filename, "w+" );
    if ( !fp )
    {
        printf( "creation of %s in folder %s failed, error %d\n", filename, foldername, errno );
        exit( 1 );
    }

    fprintf( fp, "hello!\n" );
    fclose( fp );

    result = unlink( filename );
    if ( 0 != result )
    {
        printf( "removal of %s file failed, error %d\n", filename, errno );
        exit( 1 );
    }

    result = chdir( ".." );
    if ( 0 != result )
    {
        printf( "cd back up to parent folder .. failed, error %d\n", errno );
        exit( 1 );
    }

    result = rmdir( foldername );
    if ( 0 != result )
    {
        printf( "end of app cleanup: rmdir of folder %s failed, error %d\n", foldername, errno );
        exit( 1 );
    }

    result = chdir( foldername );
    if ( 0 == result )
    {
        printf( "cd into the removed test folder %s succeded, and it shouldn't have\n", foldername );
        exit( 1 );
    }

    printf( "tdir completed with great success\n" );
    return 0;
} //main

