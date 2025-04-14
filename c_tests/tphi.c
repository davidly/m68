#include <stdio.h>
#include <stdint.h>

int main()
{
    printf( "should tend towards 1.61803398874989484820458683436563811772030\n" );
    const uint64_t limit = 40;
    uint64_t prev2 = 1;
    uint64_t prev1 = 1;
    uint64_t last_shown = 0;

    for ( uint64_t i = 1; i <= limit; i++ )
    {
        uint64_t next = prev1 + prev2;
        prev2 = prev1;
        prev1 = next;

        if ( i == ( last_shown + 5 ) )
        {
            last_shown = i;
            printf( "  at %2u iterations: %.16lf\n", (unsigned int) i, (double) prev1 / (double) prev2 );
        }
    }

    printf( "done\n" );
    return 0;
}
