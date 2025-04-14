#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void show_local_time( const char * ptz )
{
    if ( 0 != ptz )
    {
        char * penv = (char *) malloc( strlen( ptz ) + 4 );
        strcpy( penv, "TZ=" );
        strcat( penv, ptz );
        putenv( penv );
        printf( "set tz '%s' ", penv );
    }

    const char * timezoneval = getenv( "TZ" );
    if ( 0 == timezoneval )
        timezoneval = "(null)";

    time_t tt;
    time( &tt );
    struct tm * t = localtime( &tt );

    printf( "tz: '%s', year: %d, month %d, day %d, hour %d, min %d, sec %d\n",
            timezoneval, t->tm_year + 1900, 1 + t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec );
} //show_local_time

extern "C" int main()
{
    printf( "before TZ is set: ");
    show_local_time( 0 );
    printf( "east coast time: ");
    show_local_time( "EST+5" );
    printf( "west coast time: ");
    show_local_time( "PST+8" );
    printf( "TZ=<blank>: ");
    show_local_time( "" );
    return 0;
} //main


