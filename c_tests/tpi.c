#include <stdio.h>
#include <stdint.h>

int main() {
    const int HIGH_MARK = 500; // 2800;
    int r[HIGH_MARK + 1];
    int i, k;
    int b, d;
    int iteration;

    const int iterations = 1;

    for ( iteration = 0; iteration < iterations; iteration++ ) {
        int c = 0;

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
                int result = c + d / 10000;
                if ( result < 1000 ) // workaround for old apple printf bug
                    printf( "0%.3d", result );
                else
                    printf( "%.4d", c + d / 10000 );
                fflush( stdout );
            }
            c = d % 10000;
        }
    }

    printf( "\n" );
    return 0;
}
