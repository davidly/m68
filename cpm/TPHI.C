#include <stdio.h>

typedef unsigned long uint32_t;
typedef long int32_t;

int main()
{
    int32_t limit = 25;
    int32_t prev2 = 1;
    int32_t prev1 = 1;
    int32_t last_shown = 0;
    int32_t i, next;
    printf( "should tend towards 1.61803398874989484820458683436563811772030\n" );

    for ( i = 1; i <= limit; i++ )
    {
        next = prev1 + prev2;
        prev2 = prev1;
        prev1 = next;

        if ( i == ( last_shown + 5 ) )
        {
            last_shown = i;
            printf( "  at %2u iterations: %.16f\n", (unsigned int) i, (float) prev1 / (float) prev2 );
        }
    }

    printf( "done\n" );
    return 0;
}
