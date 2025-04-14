// sample code taken from stackoverflow.

#include <stdio.h>
#include <setjmp.h>

static jmp_buf buf;

void second(void)
{
    printf( "second\n" );    // prints
    longjmp( buf, 1 );       // jumps back to where setjmp was called - making setjmp now return 1
}

void first(void)
{
    second();
    printf( "first (if this prints there is a bug)\n" );
}

int main()
{   
    if ( ! setjmp( buf ) )
        first();          // when executed, setjmp returns 0
    else                  // when longjmp jumps back, setjmp returns 1
        printf( "back in main\n" ); // prints

    printf( "falling out of main\n" );
    return 0;
}
