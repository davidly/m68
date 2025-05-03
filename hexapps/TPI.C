#include <stdio.h>

#define HIGH_MARK 500 /* 2800 */

static long r[HIGH_MARK + 1];

int main() {
    long i, k, c;
    long b, d, v;
    long iteration;
    int result;
    int iterations = 1;

    for ( iteration = 0; iteration < iterations; iteration++ ) {
        c = 0;

        for (i = 0; i < HIGH_MARK; i++) {
            r[i] = 2000;
        }

        for (k = HIGH_MARK; k > 0; k -= 14) {
            d = 0;

            i = k;
            for (;;) {
                d += r[i] * 10000;
                b = 2 * i - 1;
                r[i] = d % b;
                d /= b;
                i--;
                if (i == 0) break;
                d *= i;
            }
            if ( iteration == ( iterations - 1 ) )
            {
                // workaround for printf bug in 68k IDE environment: use a local int
                result = (int) ( c + d / 10000 );
                printf( "%.4d", result );
            }
            c = d % 10000;
        }
    }

    printf( "\n" );
    return 0;
}