#include <stdio.h>
#include <stdint.h>
#include <cstring>
#include <unistd.h>

#ifndef _countof
#define _countof( X ) ( sizeof( X ) / sizeof( X[0] ) )
#endif

#pragma pack( push, 1 )
struct SMany
{
    uint8_t ui8;
    uint64_t ui64;
    uint16_t ui16;
    uint32_t ui32;
    int8_t i8;
    int64_t i64;
    int16_t i16;
    int32_t i32;
};

static SMany amany_before[ 10 ];
static SMany amany[ 20 ];
static SMany amany_after[ 10 ];
#pragma pack(pop)

uint64_t one_bits( uint64_t bits )
{
    if ( 64 == bits )
        return ~ 0ull;
    return ( ( 1ull << bits ) - 1 );
}

template <class T> void validate_array( T * a, size_t c )
{
    T sum = 0;
    for ( size_t i = 0; i < c; i++ )
        sum += a[ i ];

    uint64_t ones = one_bits( 8 * sizeof( T ) );
    uint64_t result = (uint64_t) sum & ones;
    if ( 8 == sizeof( T ) )
        printf( "sum: %#llx\n", result );
    else
        printf( "sum: %#x\n", (uint32_t) result );
} //validate_array

template <class T> void test_array( T * a, size_t c )
{
    for ( size_t i = 0; i < c; i++ )
        a[ i ] = (T) ( (T) i - ( (T) c / 2 ) );

    validate_array( a, c );
} //test_array

static char * appendHexNibble( char * p, uint8_t val )
{
    *p++ = ( val <= 9 ) ? val + '0' : val - 10 + 'a';
    return p;
} //appendHexNibble

static char * appendHexByte( char * p, uint8_t val )
{
    p = appendHexNibble( p, ( val >> 4 ) & 0xf );
    p = appendHexNibble( p, val & 0xf );
    return p;
} //appendBexByte

static char * appendHexWord( char * p, uint16_t val )
{
    p = appendHexByte( p, ( val >> 8 ) & 0xff );
    p = appendHexByte( p, val & 0xff );
    return p;
} //appendHexWord

void ShowBinaryData( uint8_t * pData, uint32_t length, uint32_t indent )
{
    int32_t offset = 0;
    int32_t beyond = length;
    const int32_t bytesPerRow = 32;
    uint8_t buf[ bytesPerRow ];
    char acLine[ 200 ];

    while ( offset < beyond )
    {
        char * pline = acLine;

        for ( uint32_t i = 0; i < indent; i++ )
            *pline++ = ' ';

        pline = appendHexWord( pline, (uint16_t) offset );
        *pline++ = ' ';
        *pline++ = ' ';

        int32_t end_of_row = offset + bytesPerRow;
        int32_t cap = ( end_of_row > beyond ) ? beyond : end_of_row;
        int32_t toread = ( ( offset + bytesPerRow ) > beyond ) ? ( length % bytesPerRow ) : bytesPerRow;
        memcpy( buf, pData + offset, toread );
        uint32_t extraSpace = 2;

        for ( int32_t o = offset; o < cap; o++ )
        {
            pline = appendHexByte( pline, buf[ o - offset ] );
            *pline++ = ' ';
            if ( ( bytesPerRow > 16 ) && ( o == ( offset + 15 ) ) )
            {
                *pline++ = ':';
                *pline++ = ' ';
                extraSpace = 0;
            }
        }

        uint32_t spaceNeeded = extraSpace + ( ( bytesPerRow - ( cap - offset ) ) * 3 );

        for ( uint32_t sp = 0; sp < ( 1 + spaceNeeded ); sp++ )
            *pline++ = ' ';

        for ( int32_t o = offset; o < cap; o++ )
        {
            char ch = buf[ o - offset ];

            if ( (int8_t) ch < ' ' || 127 == ch )
                ch = '.';

            *pline++ = ch;
        }

        offset += bytesPerRow;
        *pline = 0;
        printf( "%s\n", acLine );
    }
} //ShowBinaryData

void test_many()
{
    memset( amany_before, 0, sizeof( amany_before ) );
    memset( amany_after, 0, sizeof( amany_after ) );

    for ( size_t i = 0; i < _countof( amany ); i++ )
    {
        SMany & m = amany[ i ];
        m.ui8 = (uint8_t) i;
        m.ui64 = (uint64_t) i * 8;
        m.ui16 = (uint16_t) i * 2;
        m.ui32 = (uint32_t) i * 4;
        m.i8 = - (int8_t) i;
        m.i64 = - (int64_t) i * 8;
        m.i16 = - (int16_t) i * 2;
        m.i32 = - (int32_t) i * 4;
    }

    memset( amany_before, 0, sizeof( amany_before ) );
    memset( amany_after, 0, sizeof( amany_after ) );

    for ( size_t i = 0; i < _countof( amany ); i++ )
    {
        SMany & m = amany[ i ];

        if ( m.ui8 != i )
            printf( "error: i %u, ui8 is %llu, not %llu\n", (int) i, (uint64_t) m.ui8, (uint64_t) i );
        if ( m.ui64 != (uint64_t) i * 8 )
            printf( "error: i %u, ui64 is %llu, not %llu\n", (int) i, (uint64_t) m.ui64, (uint64_t) i * 8 );
        if ( m.ui16 != (uint16_t) i * 2 )
            printf( "error: i %u, ui16 is %llu, not %llu\n", (int) i, (uint64_t) m.ui16, (uint64_t) i * 2 );
        if ( m.ui32 != (uint32_t) i * 4 )
            printf( "error: i %u, ui32 is %llu, not %llu\n", (int) i, (uint64_t) m.ui32, (uint64_t) i * 4 );

        if ( m.i8 != - (int8_t) i )
            printf( "error: i %u, i8 is %lld, not %lld\n", (int) i, (int64_t) m.i8, - (int64_t) i );
        if ( m.i64 != - (int64_t) i * 8 )
            printf( "error: i %u, i64 is %lld, not %lld\n", (int) i, (int64_t) m.ui64, - (int64_t) i * 8 );
        if ( m.i16 != - (int16_t) i * 2 )
            printf( "error: i %u, i16 is %lld, not %lld\n", (int) i, (int64_t) m.ui16, - (int64_t) i * 2 );
        if ( m.i32 != - (int32_t) i * 4 )
            printf( "error: i %u, i32 is %lld, not %lld\n", (int) i, (int64_t) m.ui32, - (int64_t) i * 4 );
    }

    ShowBinaryData( (uint8_t *) amany, sizeof( amany ), 4 );
} //test_many

int main( int argc, char * argv[] )
{
    int8_t ai8[ 16 ];
    test_array( ai8, _countof( ai8 ) );
    uint8_t aui8[ 16 ];
    test_array( aui8, _countof( aui8 ) );

    int16_t ai16[ 16 ];
    test_array( ai16, _countof( ai16 ) );
    uint16_t aui16[ 16 ];
    test_array( aui16, _countof( aui16 ) );

    int32_t ai32[ 16 ];
    test_array( ai32, _countof( ai32 ) );
    uint32_t aui32[ 16 ];
    test_array( aui32, _countof( aui32 ) );

    int64_t ai64[ 16 ];
    test_array( ai64, _countof( ai64 ) );
    uint64_t aui64[ 16 ];
    test_array( aui64, _countof( aui64 ) );

    // validate again to check for memory trashing

    validate_array( ai8, _countof( ai8 ) );
    validate_array( aui8, _countof( aui8 ) );
    validate_array( ai16, _countof( ai16 ) );
    validate_array( aui16, _countof( aui16 ) );
    validate_array( ai32, _countof( ai32 ) );
    validate_array( aui32, _countof( aui32 ) );
    validate_array( ai64, _countof( ai64 ) );
    validate_array( aui64, _countof( aui64 ) );

    test_array( ai16, _countof( ai16 ) );
    test_array( aui16, _countof( aui16 ) );
    test_array( ai8, _countof( ai8 ) );
    test_array( aui8, _countof( aui8 ) );

    test_many();

    printf( "tarray completed with great success\n" );
    return 0;
} //main

