#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern char **environ;

int main()
{
    printf( "environ at start: %p\n", environ );
    for ( char ** s = environ; *s; s++ )
         printf("  %p: %s\n", *s, *s);

    char ac[100];
    strcpy( ac, "MYVAL=toast!" );
    putenv( ac );

    char * pval = getenv( "MYVAL" );

    printf( "pval: %p\n", pval );

    if ( 0 != pval )
        printf( "value: %s\n", pval );

    const char * ptz = "PST+8";
    char * penv = (char *) malloc( strlen( ptz ) + 4 );
    strcpy( penv, "TZ=" );
    strcat( penv, ptz );
    putenv( penv );

    printf( "environ at end: %p\n", environ );
    for ( char ** s = environ; *s; s++ )
         printf("  %s\n", *s);

    return 0;
}
