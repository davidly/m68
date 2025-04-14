#include <stdio.h>
#include <exception>

using namespace std;

class CUnwound
{
    private:
        int x;
    public:
        CUnwound() : x( 44 ) {}
        ~CUnwound() { printf( "I am unwound, x should be 44: %d\n", x ); }
        void set( int val ) { x = val; }
};

struct exceptional : std::exception
{
    const char * what() const noexcept { return "exceptional"; }
};

// without this, the call to operator new is optimized out

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
            int volatile * myarray = new int[ 1000000 ];
            if ( myarray )
                successful++;
            else
                printf( "new failed but didn't raise!?!\n" );
            //printf( "allocation %zd succeeded %p\n", i, myarray );
        }
        printf( "large allocations succeeded?!? (%d)\n", successful );
    }
    catch ( exception & e )
    {
        printf( "caught a standard execption: %s\n", e.what() );
        fflush( stdout );
    }
    catch ( ... )
    {
        printf( "caught generic exception\n" ); 
    }

    printf( "leaving main\n" );
    return 0;
}

