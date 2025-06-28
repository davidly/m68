#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <stdint.h>

#define allocs 69
int logging = 1;

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
    int64_t offset = 0;
    int64_t beyond = length;
    const int64_t bytesPerRow = 32;
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

        int64_t end_of_row = offset + bytesPerRow;
        int64_t cap = ( end_of_row > beyond ) ? beyond : end_of_row;
        int64_t toread = ( ( offset + bytesPerRow ) > beyond ) ? ( length % bytesPerRow ) : bytesPerRow;
        memcpy( buf, pData + offset, toread );
        uint64_t extraSpace = 2;

        for ( int64_t o = offset; o < cap; o++ )
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

        uint64_t spaceNeeded = extraSpace + ( ( bytesPerRow - ( cap - offset ) ) * 3 );

        for ( uint64_t sp = 0; sp < ( 1 + spaceNeeded ); sp++ )
            *pline++ = ' ';

        for ( int64_t o = offset; o < cap; o++ )
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

char * memset_x( char * p, int v, int c )
{
    unsigned char * pc = (unsigned char *) p;
    unsigned char val = (unsigned char) ( v & 0xff );
    int i;

    if ( 0 == p )
    {
        printf( "request to memset a null pointer\n" );
        exit( 1 );
    }

    if ( logging )
#ifdef CPMTIME
        printf( "  memset p %u, val %x, c %d\n", p, val, c );
#else
        printf( "  memset p %p, val %#x, count of bytes %d\n", p, val, c );
#endif

    for ( i = 0; i < c; i++ )
        *pc++ = val;
    return p;
}

void chkmem( char * p, int v, int c )
{
    unsigned char * pc = (unsigned char *) p;
    unsigned char val = (unsigned char) ( v & 0xff );
    int i;

    if ( 0 == p )
    {
        printf( "request to chkmem a null pointer\n" );
        exit( 1 );
    }

    for ( i = 0; i < c; i++ )
    {
        if ( *pc != val )
        {
#ifdef CPMTIME
            printf( "memory isn't as expected! p %u, v %d, c %d, *pc %d\n",p, v, c, *pc );
#else
            printf( "memory isn't as expected! p %p i %d, of count %d, val expected %#x, val found %#x\n",
                    p, i, c, (unsigned char) v, (unsigned char) *pc );
            ShowBinaryData( (uint8_t *) p, c, 4 );
#endif
            exit( 1 );
        }
        pc++;
    }
}

int main( int argc, char * argv[] )
{
    int i, cb, j;
    char * ap[ allocs ];
    char * pc;

    logging = ( argc > 1 );
    pc = argv[ 0 ]; /* evade compiler warning */

    for ( j = 0; j < 10; j++ )
    {
        if ( logging )
            printf( "in alloc mode pass %d\n", j );

        for ( i = 0; i < allocs; i++ )
        {
            cb = 8 + ( i * 10 );
            int cb_calloc = cb + 5;
            if ( logging )
                printf( "  i, cb, cb_calloc: %d %d %d\n", i, cb, cb_calloc );

            pc = (char *) calloc( cb_calloc, 1 );
            chkmem( pc, 0, cb_calloc );
            memset_x( pc, 0xcc, cb_calloc );

            ap[ i ] = (char *) malloc( cb );
            memset_x( ap[ i ], 0xaa, cb );

            chkmem( pc, 0xcc, cb_calloc );
            free( pc );
        }

        if ( logging )
            printf( "in free mode, even first\n" );

        for ( i = 0; i < allocs; i += 2 )
        {
            cb = 8 + ( i * 10 );
            int cb_calloc = cb + 3;
            if ( logging )
                printf( "  i, cb, cb_calloc: %d %d %d\n", i, cb, cb_calloc );

            pc = (char *) calloc( cb_calloc, 1 );
            chkmem( pc, 0, cb_calloc );
            memset_x( pc, 0xcc, cb_calloc );

            chkmem( ap[ i ], 0xaa, cb );
            memset_x( ap[ i ], 0xff, cb );
            free( ap[ i ] );

            chkmem( pc, 0xcc, cb_calloc );
            free( pc );
        }

        if ( logging )
            printf( "in free mode, now odd\n" );

        for ( i = 1; i < allocs; i += 2 )
        {
            cb = 8 + ( i * 10 );
            int cb_calloc = cb + 7;
            if ( logging )
                printf( "  i, cb, cb_calloc: %d %d %d\n", i, cb, cb_calloc );

            pc = (char *) calloc( cb_calloc, 1 );
            if ( logging )
                printf( "  calloc'ed memory at %p\n", pc );
            chkmem( pc, 0, cb_calloc );

            chkmem( ap[ i ], 0xaa, cb );
            memset_x( ap[ i ], 0xff, cb );
            free( ap[ i ] );

            chkmem( pc, 0, cb_calloc );
            free( pc );
        }
    }

    printf( "tm has completed with great success\n" );
    return 0;
}
