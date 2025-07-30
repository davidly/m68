// test various directory-related operations

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>

#ifndef PATH_MAX // newlib doesn't define this
#define PATH_MAX 255
#include "realpath.c"
#endif

int main( int argc, char * argv[] )
{
    const char * foldername = "tdir_testfolder";
    const char * filename = "tdir_testfile.txt";

    int result = mkdir( foldername, S_IRWXU | S_ISVTX );
    if ( 0 != result )
    {
        printf( "mkdir failed, error %d\n", errno );
        printf( "perhaps folder '%s' exists from a prior run; deleting it\n", foldername );
        fflush( stdout );

        // first try to delete the file if it exists. ignore the result(s)
        if ( 0 == chdir( foldername ) )
        {
            unlink( filename );
            result = chdir( ".." );
            if ( 0 != result )
            {
                printf( "for some reason chdir .. failed: %d\n", result );
                exit( 1 );
            }
        }

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

    char cwd_original[ PATH_MAX ];
    char * pcwd_original = getcwd( cwd_original, sizeof( cwd_original ) );
    if ( 0 == pcwd_original )
    {
        printf( "unable to retrieve original current directory, error %d\n", errno );
        exit( 1 );
    }

    result = chdir( foldername );
    if ( 0 != result )
    {
        printf( "chdir into the test folder %s failed, error %d\n", foldername, errno );
        exit( 1 );
    }

    char cwd[ PATH_MAX ];
    char * pcwd = getcwd( cwd, sizeof( cwd ) );
    if ( 0 == pcwd )
    {
        printf( "unable to retrieve current (child) directory, error %d\n", errno );
        exit( 1 );
    }

    char * pslash = strrchr( cwd, '/' );
    if ( 0 == pslash )
    {
        printf( "unable to find a slash in cwd '%s'\n", cwd );
        exit( 1 );
    }

    if ( strcmp( pslash + 1, foldername ) )
    {
        printf( "cwd '%s' isn't expected value '%s'\n", pslash + 1, foldername );
        exit( 1 );
    }

    FILE * fp = fopen( filename, "w+" );
    if ( !fp )
    {
        printf( "creation of %s in folder %s failed, error %d\n", filename, foldername, errno );
        exit( 1 );
    }

    fprintf( fp, "aespa winter\n" );
    fflush( fp );
    fclose( fp );

    struct stat st;
    memset( & st, 0, sizeof( struct stat ) );
    result = stat( filename, &st );
    if ( 0 != result )
    {
        printf( "stat on file '%s' failed, error %d\n", filename, errno );
        exit( 1 );
    }

    if ( S_ISDIR( st.st_mode ) )
    {
        printf( "stat claims file '%s' is a directory\n", filename );
        exit( 1 );
    }

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

    memset( & st, 0, sizeof( struct stat ) );
    result = stat( foldername, &st );
    if ( 0 != result )
    {
        printf( "stat on folder '%s' failed, error %d\n", foldername, errno );
        exit( 1 );
    }

    if ( !S_ISDIR( st.st_mode ) )
    {
        printf( "stat claims directory '%s' isn't a directory\n", foldername );
        exit( 1 );
    }

    char resolved[ PATH_MAX ];
    char * presolved = realpath( foldername, resolved );
    if ( 0 == presolved )
    {
        printf( "realpath failed, error %d\n", errno );
        exit( 1 );
    }

    if ( strcmp( presolved, pcwd ) )
    {
        printf( "realpath of child folder '%s' doesn't match getcwd result '%s'\n", presolved, pcwd );
        exit( 1 );
    }

    result = rmdir( foldername );
    if ( 0 != result )
    {
        printf( "end of app cleanup: rmdir of folder '%s' failed, error %d\n", foldername, errno );
        exit( 1 );
    }

    result = chdir( foldername );
    if ( 0 == result )
    {
        printf( "cd into the removed test folder '%s' succeded, and it shouldn't have\n", foldername );
        exit( 1 );
    }

    char cwd_final[ PATH_MAX ];
    char * pcwd_final = getcwd( cwd_final, sizeof( cwd_final ) );
    if ( 0 == pcwd_final )
    {
        printf( "unable to retrieve final current directory, error %d\n", errno );
        exit( 1 );
    }

    if ( strcmp( cwd_original, cwd_final ) )
    {
        printf( "original directory '%s' isn't the same as final directory '%s'\n", cwd_original, cwd_final );
        exit( 1 );
    }

    printf( "tdir completed with great success\n" );
    return 0;
} //main

