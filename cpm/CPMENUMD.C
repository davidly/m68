/*
    This app tests cp/m 68k file system enumeration.
    It also tests the memory allocator and qsort for fun.
    x_functions exist because the C runtime doesn't provide the normal versions.
*/

#include <stdio.h>

// the C runtime provides __BDOS

typedef unsigned char uint8_t;
typedef char int8_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned long uint32_t;
typedef long int32_t;
typedef int bool;
typedef unsigned int size_t;

#define false 0
#define true 1
#define INT_MAX 32767

extern uint8_t _start;  // app entry point, 128 bytes above the default DMA address

struct FCBCPM68K // file control block for cp/m
{
    uint8_t dr;         // drive
    char n[ 8 ];        // filename
    char t[ 3 ];        // file type
    uint8_t ex;         // current extent number. normally set to 0 by user but is in range 0-31
    uint8_t s1;
    uint8_t s2;         // reserved for system use. set to 0 for open/make/search
    uint8_t rc;         // record count, reserved for system use
    uint8_t d[ 16 ];
    uint8_t cr;         // current reacord to be read or written for sequential read/write operations. apps must set approprately.
    uint8_t r0;         // (optional if app is doing random I/O) random record number. most significant byte in r0 then r1 then r2 as a 24-bit record
    uint8_t r1;         // the byte order is the opposite of CP/M 2.2 for 8080 plus r2 is actually used!
    uint8_t r2;
};

struct FCBCPM68K the_fcb;
char * malloc();

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
} //x_bsearch

int enumerate( fname, ftype ) char * fname; char * ftype;
{
    struct FCBCPM68K * result_fcb;
    int result, count, i, len;
    int list_len = 0;
    char * pthis, * presult;
    char file[ 13 ];
    static char * list[ 200 ];

    x_memcpy( the_fcb.n, fname, 8 );
    x_memcpy( the_fcb.t, ftype, 3 );

    result = __BDOS( 17, & the_fcb );
    count = 0;

    while ( result >= 0 && result <= 3 )
    {
        count++;
        result_fcb = (struct FCBCPM68K *) ( & _start - 128 + ( result * 32 ) );
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
        
        result = __BDOS( 18, & the_fcb );
    }

    qsort( list, list_len, sizeof( char * ), do_compare );

    if ( !strcmp( fname, "????????" ) && !strcmp( ftype, "68K" ) )
    {
        char * pkey = "CPMENUMD.68K";
        char ** presult = x_bsearch( &pkey, list, list_len, sizeof( char * ), do_compare );
        if ( 0 != presult )
            printf( "  found this executable! %s\n", *presult );
    }

    printf( "  result from find first: %d\n", result );
    for ( i = 0; i < list_len; i++ )
    {
        printf( "  file %3d: %s\n", i, list[ i ] );
        free( list[ i ] );
    }

    return 0;
}

int main()
{
    x_memset( & the_fcb, 0, sizeof( the_fcb ) );

#if 0 // these will vary too much between systems
    printf( "finding all files\n" );
    enumerate( "????????", "???" );

    printf( "finding files that start with A\n" );
    enumerate( "A???????", "???" );

    printf( "finding C files\n" );
    enumerate( "????????", "C  " );

    printf( "finding PAS files\n" );
    enumerate( "????????", "PAS" );
#endif

    printf( "finding 68K files\n" );
    enumerate( "????????", "68K" );

    printf( "finding XYZ files\n" );
    enumerate( "????????", "XYZ" );

    printf( "finding TXT files\n" );
    enumerate( "????????", "TXT" );

    printf( "cp/m enumerate directory completed with great success\n" );
    return 0;
}
