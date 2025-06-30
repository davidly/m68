/*
    This app tests cp/m 68k file system enumeration.
    It also tests the memory allocator and qsort for fun.
    x_functions exist because the C runtime doesn't provide the normal versions.
*/

#include <stdio.h>

char * malloc();

typedef unsigned char uint8_t;
typedef char int8_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned long uint32_t;
typedef long int32_t;
typedef int bool;

#define false 0
#define true 1

#ifdef CPM80
#define DEFAULT_DMA 128
#else
#define bdos __BDOS
extern uint8_t _start;
#define DEFAULT_DMA ( ( & _start ) - 128 )
#endif

struct FCBCPM 
{
    uint8_t dr;      
    char n[ 8 ];     
    char t[ 3 ];     
    uint8_t ex;      
    uint8_t s1;
    uint8_t s2;      
    uint8_t rc;      
    uint8_t d[ 16 ];
    uint8_t cr;      
    uint8_t r0;      
    uint8_t r1;      
#ifndef CPM80
    uint8_t r2;   /* CP/M 68K and >= CP/M 80 v3 */
#endif
};

char * x_strchr( str, c ) char * str; char c;
{
    while ( *str )
    {
        if ( *str == c )
            return str; 
        str++;
    }

    return 0;
}

char * x_memset( p, v, c ) char * p; int v; int c;
{
    int i;
    char val = (char) v;
    char * pc = p;
    for ( i = 0; i < c; i++ )
        *pc++ = val;
    return p;
}

char * x_memcpy( a, b, c ) char * a; char * b; int c;
{
    char * dst = a;
    while ( c > 0 )
    {
        *a++ = *b++;
        c--;
    }
    return dst;
}

bool fcb_initialize( pfcb, pfilename ) struct FCBCPM * pfcb; char * pfilename;
{
    char * pdot, * ptype;
    int len, nlen, tlen;

    x_memset( pfcb, 0, sizeof( struct FCBCPM ) );
    x_memset( pfcb->n, ' ', 8 );
    x_memset( pfcb->t, ' ', 3 );

    len = strlen( pfilename );
    pdot = x_strchr( pfilename, '.' );
    if ( pdot )
    {
        nlen = (int) ( pdot - pfilename );
        if ( nlen > 8 )
            return false;
        x_memcpy( pfcb->n, pfilename, nlen );
        ptype = pdot + 1;
        tlen = strlen( ptype );
        if ( tlen > 3 )
            return false;
        x_memcpy( pfcb->t, ptype, tlen );
    }
    else
    {
        if ( len > 8 )
            return false;
        x_memcpy( pfcb->n, pfilename, len );
    }

    return true;
}

uint32_t file_size( pfilename ) char * pfilename;
{
    struct FCBCPM size_fcb;
    uint32_t size = 0;
    int result;

    fcb_initialize( & size_fcb, pfilename );
    result = bdos( 35, & size_fcb );
    if ( 0 == result )
    {
#ifdef CPM80
        /* CP/M 80 doesn't use r2 and it's little-endian */
        size = ( (uint32_t) size_fcb.r0 + ( ( (uint32_t) size_fcb.r1 ) << 8 ) );
#else
        /* CP/M 68K uses r2 and it's big-endian */
        size = ( (uint32_t) size_fcb.r2 + ( ( (uint32_t) size_fcb.r1 ) << 8 ) + ( ( (uint32_t) size_fcb.r0 ) << 16 ) );
#endif
    }

    size <<= 7;   /* 128 bytes per record; CP/M file sizes are in multiples of the record size, not an exact byte count */
    return size;
}

char * x_strdup( a ) char * a;
{
    int len = strlen( a );
    char * result = (char *) malloc( 1 + len );
    strcpy( result, a );
    return result;
}

int do_compare( a, b ) char ** a; char ** b;
{
    return strcmp( *a, *b );
}

char ** x_bsearch( key, vbase, num, width, compare ) char ** key; char ** vbase; int num; int width; int (*compare)();
{
    char * base = (char *) vbase;
    int k, cmp;
    char * here;
    int i = 0;
    int j = num - 1;

    if ( 0 == num )
        return 0;

    do
    {
        k = ( j + i ) / 2;
        here = base + width * k;
        cmp = ( *compare )( key, here );
        if ( 0 == cmp )
        {
            while ( ( here > base ) && ( ( *compare )( key, here - width ) == 0 ) )
                here -= width;
            return here;
        }

        if ( cmp < 0 )
            j = k - 1;
        else
            i = k + 1;
    } while ( j >= i );

    return 0;
} 

int enumerate( pfile ) char * pfile;
{
#ifdef CPM80
    char * pappname = "CPMENUMD.COM";
#else
    char * pappname = "CPMENUMD.68K";
#endif

    struct FCBCPM the_fcb;
    struct FCBCPM * result_fcb;
    int result, i, len;
    int list_len = 0;
    char * pthis, ** presult;
    uint32_t fsize;
    char file[ 13 ];
    static char * list[ 200 ];

    if ( ! fcb_initialize( & the_fcb, pfile ) )
        return false;

    result = bdos( 17, & the_fcb ); // find first

    while ( result >= 0 && result <= 3 )
    {
        result_fcb = (struct FCBCPM *) ( DEFAULT_DMA + ( result * 32 ) );
        len = 0;

        for ( i = 0; i < 8; i++ )
        {
            if ( ' ' != result_fcb->n[ i ] )
                file[ len++ ] = result_fcb->n[ i ];
            else
                break;
        }

        if ( ' ' != result_fcb->t[ 0 ] )
        {
            file[ len++ ] = '.';
            for ( i = 0; i < 3; i++ )
            {
                if ( ' ' != result_fcb->t[ i ] )
                    file[ len++ ] = result_fcb->t[ i ];
                else
                    break;
            }
        }

        file[ len ] = 0;
        list[ list_len++ ] = x_strdup( file );
        if ( ( sizeof( list ) / sizeof( char * ) ) == list_len )
            break;
        
        result = bdos( 18, & the_fcb ); // find next
    }

    if ( 255 != result )
    {
        printf( "unexpected result from find first/next: %d\n", result );
        return false;
    }

    qsort( list, list_len, sizeof( char * ), do_compare );

    presult = x_bsearch( & pappname, list, list_len, sizeof( char * ), do_compare );
    if ( 0 != presult )
        printf( "  found this executable! %s\n", *presult );
    
    for ( i = 0; i < list_len; i++ )
    {
        fsize = file_size( list[ i ] );
        printf( "  file %3d: %13s  %8ld\n", i, list[ i ], (long) fsize );
        free( list[ i ] );
    }

    return true;
}

int main()
{
#ifdef CPM80
    printf( "finding all files\n" );
    enumerate( "????????.???" );

    printf( "finding files that start with A\n" );
    enumerate( "A???????.???" );

    printf( "finding C files\n" );
    enumerate( "????????.C" );

    printf( "finding PAS files\n" );
    enumerate( "????????.PAS" );
#endif

    printf( "finding COM files\n" );
    enumerate( "????????.COM" );

    printf( "finding 68K files\n" );
    enumerate( "????????.68K" );

    printf( "finding XYZ files\n" );
    enumerate( "????????.XYZ" );

    printf( "finding TXT files\n" );
    enumerate( "????????.TXT" );

    printf( "cp/m enumerate directory completed with great success\n" );
    return 0;
}
