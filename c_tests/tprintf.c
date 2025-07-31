#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>


void cppreference() // from https://en.cppreference.com/w/c/io/fprintf
{
    const char* s = "Hello";
    printf("Strings:\n"); // same as puts("Strings");
    printf(" padding:\n");
    printf("\t[%10s]\n", s);
    printf("\t[%-10s]\n", s);
    printf("\t[%*s]\n", 10, s);
    printf(" truncating:\n");
    printf("\t%.4s\n", s);
    printf("\t%.*s\n", 3, s);

    printf("Characters:\t%c %%\n", 'A');

    printf("Integers:\n");
    printf("\tDecimal:\t%i %d %.6i %i %.0i %+i %i\n",
                         1, 2,   3, 0,   0,  4,-4);
    printf("\tHexadecimal:\t%x %x %X %#x\n", 5, 10, 10, 6);
    printf("\tOctal:\t\t%o %#o %#o\n", 10, 10, 4);

    printf("Floating-point:\n");
    printf("\tRounding:\t%f %.0f %.32f\n", 1.5, 1.5, 1.3);
    printf("\tPadding:\t%05.2f %.2f %5.2f\n", 1.5, 1.5, 1.5);
    printf("\tScientific:\t%E %e\n", 1.5, 1.5);
    printf("\tHexadecimal:\t%a %A\n", 1.5, 1.5);
    printf("\tSpecial values:\t0/0=%g 1/0=%g\n", 0.0 / 0.0, 1.0 / 0.0);

    printf("Fixed-width types:\n");
    printf("\tLargest 32-bit value is %" PRIu32 " or %#" PRIx32 "\n",
                                     UINT32_MAX,     UINT32_MAX );
    printf("\tLargest 64-bit value is %" PRIu64 " or %#" PRIx64 "\n",
                                     UINT64_MAX,     UINT64_MAX );
}

int main( int argc, char * argv[] )
{
    float f = 1.01;
    printf( "float: %f\n", f );
    printf( "  %3.3f\n", f );
    printf( "  %1.1f\n", f );
    printf( "  %.4f\n", f );
    printf( "  %4.f\n", f );

    f = -6789.01234;
    printf( "float: %f\n", f );
    printf( "  %3.3f\n", f );
    printf( "  %1.1f\n", f );
    printf( "  %.4f\n", f );
    printf( "  %4.f\n", f );

    double d = 1.01;
    printf( "double: %lf\n", d );
    printf( "  %3.3lf\n", d );
    printf( "  %1.1lf\n", d );
    printf( "  %.4lf\n", d );
    printf( "  %4.lf\n", d );
    printf( "  %e\n", d );
    printf( "  %a\n", d );
    printf( "  %g\n", d );

    d = -6789.01234;
    printf( "double: %lf\n", d );
    printf( "  %3.3lf\n", d );
    printf( "  %1.1lf\n", d );
    printf( "  %.4lf\n", d );
    printf( "  %4.lf\n", d );
    printf( "  %e\n", d );
    printf( "  %a\n", d );
    printf( "  %g\n", d );

    cppreference();

    return 0;
}
