#include <stdlib.h>
#include <stdio.h>
#include <string.h>


int main()
{
    char ac[100];
    char * pval;
    strcpy( ac, "MYVAL=toast!" );
    putenv( ac );

    pval = getenv( "MYVAL" );

    printf( "pval: %p\n", pval );

    if ( 0 != pval )
        printf( "value: %s\n", pval );

    return 0;
} 
