#include <stdio.h>
#include <exception>

using namespace std;

class CUnwound
{
    private:
        int x;
    public:
        CUnwound() : x( 44 ) { printf( "CUnwould constructor\n" ); }
        ~CUnwound() { printf( "I am unwound, x should be 44: %d\n", x ); }
        void set( int val ) { x = val; }
};

struct exceptional : std::exception
{
    const char * what() const noexcept { return "exceptional"; }
};

// without disabling optimizations, the call to operator new is optimized out

#pragma GCC optimize("O0")
#pragma clang optimize off

int main()
{
    printf( "top of tex\n" );

    try
    {
        CUnwound unwound;
        printf( "throwing execption\n" );
        throw exceptional();
        unwound.set( 33 ); // should never be executed
        printf( "error: exception didn't happen\n" );
    }
    catch ( exception & e )
    {
        printf( "caught exception %s\n", e.what() );
    }

    int successful = 0;

    try
    {
        printf( "attempting large allocations\n" );
        for ( size_t i = 0; i < 1000; i++ )
        {
            int volatile * myarray = new int[ 1024 * 1024 ];
            if ( myarray )
                successful++;
            else
                printf( "error: new failed but didn't raise!?!\n" );
        }
        printf( "error: large allocations succeeded?!? (%d)\n", successful );
    }
    catch ( exception & e )
    {
        printf( "caught a standard execption: %s\n", e.what() );
        printf( "  after %d 1 megabyte allocations\n", successful );
        fflush( stdout );
    }
    catch ( ... )
    {
        printf( "caught generic exception\n" ); 
    }

    printf( "test exceptions completed with great success\n" );
    return 0;
}

