#include <stdio.h>
#include <stdint.h>
#include <atomic>
#include <cassert>

#if 0
#include <mutex>
#endif

template <class T> void validate( std::atomic<T> & a )
{
    a = 0x36;
    a++;
    assert( 0x37 == a );
    a--;
    assert( 0x36 == a );

    uint64_t v = a.fetch_add( 0x20 );
    assert( 0x36 == v );
    assert( 0x56 == a );

    v = a.fetch_sub( 0x20 );
    assert( 0x56 == v );
    assert( 0x36 == a );

    // min and max aren't in my version of g++
    //v = a.fetch_max( 0x7a );
    //assert( 0x7a == v );
    //v = a.fetch_min( 2 );
    //assert( 2 == v );

    v = a.fetch_and( 0x44 );
    assert( 4 == a );
    assert( 0x36 == v );

    a = 0x36;
    v = a.fetch_or( 1 );
    assert( 0x37 == a );
    assert( 0x36 == v );

    a = 0x36;
    v = a.fetch_xor( 4 );
    assert( 0x32 == a );
    assert( 0x36 == v );

    a = 0x36;
    T b = 0x36;
    T c = 0x37;
    bool result = a.compare_exchange_weak( b, c, std::memory_order_release, std::memory_order_relaxed );
    assert( 0x37 == a );
    assert( result );
} // validate

int main( int argc, char * argv[] )
{
    std::atomic<int8_t> i8( 0 );
    validate( i8 );
    std::atomic<uint8_t> ui8( 0 );
    validate( ui8 );
    std::atomic<int16_t> i16( 0 );
    validate( i16 );
    std::atomic<uint16_t> ui16( 0 );
    validate( ui16 );
    std::atomic<int32_t> i32( 0 );
    validate( i32 );
    std::atomic<uint32_t> ui32( 0 );
    validate( ui32 );
    std::atomic<int64_t> i64( 0 );
    validate( i64 );
    std::atomic<uint64_t> ui64( 0 );
    validate( ui64 );

#if 0
    // armos only has one core and supports one thread. but code will uses mutexes so validate they don't fail
    
    std::mutex mtx;
    mtx.lock();
    mtx.unlock();
    bool result = mtx.try_lock();
    assert( true );
    mtx.unlock();
#endif

    printf( "test atomic completed with great success\n" );
    return 0;
} //main
