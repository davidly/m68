#include <stdio.h>
#include <stdlib.h>

class CTest
{
    private:
        int foo;
    public:
        CTest() : foo( 666 )
        {
            printf( "in CTest constructor\n" );
        }

        ~CTest()
        {
            printf( "in ~CTest destructor\n" );
        }

        int get_foo() { return foo; }
};

CTest ctest;

int main( int argc, char * argv[], char * envp[] )
{
    printf( "top main\n" );

    printf( "value of ctest::foo: %d\n", ctest.get_foo() );

    printf( "end of main\n" );
    exit( 37 );
} //main

