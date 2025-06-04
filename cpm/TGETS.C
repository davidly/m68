#include <stdio.h>

char * gets();

int main( argc, argv ) int argc; char * argv[];
{
    char acbuf[ 100 ];
    char * result;

    result = gets( acbuf );
    if ( 0 == result )
        printf( "gets failed\n" );
    else
    {
        while ( *result )
        {
            printf( "read: '%c' == %u == %x\n", *result, *result, *result );
            result++;
        }
    }
    printf( "tgets completed with great success\n" );
    return 0;
}
