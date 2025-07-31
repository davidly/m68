#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <ctype.h>
#include <cerrno>
#include <cstring>

int case_insensitive_compare( const char *str1, const char *str2 )
{
    while ( *str1 && *str2 )
    {
        int diff = tolower( (unsigned char) *str1 ) - tolower( (unsigned char) *str2 );
        if ( 0 != diff )
            return diff;

        str1++;
        str2++;
     }
     return ( tolower( (unsigned char) *str1 ) - tolower( (unsigned char) *str2 ) );
} //case_insensitive_compare

int main( int argc, char * argv[] )
{
    char acApp[ 60 ];
    strcpy( acApp, argv[ 0 ] );

    DIR * dir = opendir( "." );
    if ( 0 == dir )
    {
        printf( "can't open current folder, error %d\n", errno );
        return -1;
    }

    int count = 0;
    bool expected_found = false;

    do
    {
        struct dirent * entry = readdir( dir );
        if ( !entry )
            break;

        if ( ( !case_insensitive_compare( acApp, entry->d_name ) ) || ( !strcmp( "..", entry->d_name ) ) )
            expected_found = true;

        if ( argc > 1 )
            printf( "entry: '%s'\n", entry->d_name );
        count++;
    } while( true );

    if ( !expected_found )
    {
        printf( "error: no expected file found in enumeration out of %d files returned\n", count );
        return 1;
    }

    int ret = closedir( dir );
    if ( 0 != ret )
    {
        printf( "error: closedir after enumeration failed, errno: %d\n", errno );
        return 1;
    }

    printf( "linux file system enumeration completed with great success\n" );
    return 0;
} // main
