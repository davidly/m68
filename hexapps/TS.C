#include <stdio.h>
#include <malloc.h>

#ifndef MWC
#include <string.h>
#include <stdlib.h>
#endif

typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned long uint32_t;
typedef signed long int32_t;
typedef int bool;
enum { false, true };

void swap( char * a, char * b )
{
    char c = *a;
    *a = *b;
    *b = c;
}

void reverse( char str[], int length )
{
    int start = 0;
    int end = length - 1;
    while ( start < end )
    {
        swap( ( str + start ), ( str + end ) );
        start++;
        end--;
    }
}

#define myabs( x ) ( x < 0 ) ? ( -x ) : ( x )

#if defined( _MSC_VER ) || defined(WIN64)

void riscv_print_text( const char * p )
{
    printf( "%s", p );
}
#endif

#if 1

void show_result_i8( int8_t x ) { printf( "sizeof int8_t: %d, result %x\n", sizeof( x ), x ); }
void show_result_ui8( uint8_t x ) { printf( "sizeof uint8_t: %d, result %x\n", sizeof( x ), x ); }
void show_result_i16( int16_t x ) { printf( "sizeof int16_t: %d, result %x\n", sizeof( x ), x ); }
void show_result_ui16( uint16_t x ) { printf( "sizeof uint16_t: %d, result %x\n", sizeof( x ), x ); }
void show_result_i32( int32_t x ) { printf( "sizeof int32_t: %d, result %lx\n", sizeof( x ), x ); }
void show_result_ui32( uint32_t x ) { printf( "sizeof uint32_t: %d, result %lx\n", sizeof( x ), x ); }
const char bc( bool x ) { return x ? 't' : 'f'; }
void show_result_bool( bool a, bool b, bool c, bool d, bool e )
{
    printf( "%c, %c, %c, %c, %c\n", bc( a ), bc( b ), bc( c ), bc( d ), bc( e ) );
}

#else
template <class T> void show_result( T x )
{
    if ( 4 == sizeof( T ) )
        printf( "sizeof T: %d, result: %x\n", sizeof( T ), x );
    else
        printf( "sizeof T: %d, result: %llx\n", sizeof( T ), x );
}
#endif

int main()
{
    uint8_t * pb;
    int8_t i8;
    uint8_t ui8;
    int16_t i16;
    uint16_t ui16;
    int32_t i32;
    uint32_t ui32;
    bool f0, f1, f2, f3, f4;
    printf( "top of app\n" );

    pb = (uint8_t *) malloc( 500 );
    memset( pb, 0, 500 );
    free( pb );

    printf( "print an int %d\n", (int32_t) 27 );

    i8 = -1;
    i8 >>= 1;
    show_result_ui8( (uint8_t) i8 );

    ui8 = 0xff;
    ui8 >>= 1;
    show_result_ui8( ui8 );

    i16 = -1;
    i16 >>= 1;
    show_result_ui16( (uint16_t) i16 );

    ui16 = 0xffff;
    ui16 >>= 1;
    show_result_ui16( ui16 );

    i32 = -1;
    i32 >>= 1;
    show_result_ui32( (uint32_t) i32 );

    ui32 = 0xffffffff;
    ui32 >>= 1;
    show_result_ui32( ui32 );

#if 0
    int64_t i64 = -1;
    i64 >>= 1;
    show_result( i64 );

    uint64_t ui64 = 0xffffffffffffffff;
    ui64 >>= 1;
    show_result( ui64 );
#endif

    printf( "now test left shifts\n" );

    i8 = -1;
    i8 <<= 1;
    show_result_ui8( (uint8_t) i8 );

    ui8 = 0xff;
    ui8 <<= 1;
    show_result_ui8( ui8 );

    i16 = -1;
    i16 <<= 1;
    show_result_ui16( (uint16_t) i16 );

    ui16 = 0xffff;
    ui16 <<= 1;
    show_result_ui16( ui16 );

    i32 = -1;
    i32 <<= 1;
    show_result_ui32( (uint32_t) i32 );

    ui32 = 0xffffffff;
    ui32 <<= 1;
    show_result_ui32( ui32 );

#if 0
    i64 = -1;
    i64 <<= 1;
    show_result( i64 );

    ui64 = 0xffffffffffffffff;
    ui64 <<= 1;
    show_result( ui64 );
#endif

    printf( "now test comparisons\n" );

    f0 = i8 == (int8_t) ui8; /*aztec requires a cast to work! */
    f1 = i8 > ui8;
    f2 = i8 >= (int8_t) ui8; /*aztec requires a cast to work! */
    f3 = i8 < (int8_t) ui8;  /*aztec requires a cast to work! */
    f4 = i8 <= ui8;
    show_result_bool( f0, f1, f2, f3, f4 );

    f0 = i16 == ui16;
    f1 = i16 > ui16;
    f2 = i16 >= ui16;
    f3 = i16 < ui16;
    f4 = i16 <= ui16;
    show_result_bool( f0, f1, f2, f3, f4 );

    f0 = i32 == ui32;
    f1 = i32 > ui32;
    f2 = i32 >= ui32;
    f3 = i32 < ui32;
    f4 = i32 <= ui32;
    show_result_bool( f0, f1, f2, f3, f4 );

#if 0
    f0 = i64 == ui64;
    f1 = i64 > ui64;
    f2 = i64 >- ui64;
    f3 = i64 < ui64;
    f4 = i64 <= ui64;
    show_result_bool( f0, f1, f2, f3, f4 );
#endif

    f0 = i8 == i16;
    f1 = i8 > i16;
    f2 = i8 >= i16;
    f3 = i8 < i16;
    f4 = i8 <= i16;
    show_result_bool( f0, f1, f2, f3, f4 );

    f0 = i16 == i32;
    f1 = i16 > i32;
    f2 = i16 >= i32;
    f3 = i16 < i32;
    f4 = i16 <= i32;
    show_result_bool( f0, f1, f2, f3, f4 );

#if 0
    f0 = i32 == i64;
    f1 = i32 > i64;
    f2 = i32 >= i64;
    f3 = i32 < i64;
    f4 = i32 <= i64;
    show_result( f0, f1, f2, f3, f4 );

    f0 = i64 == ui8;
    f1 = i64 > ui8;
    f2 = i64 >- ui8;
    f3 = i64 < ui8;
    f4 = i64 <= ui8;
    show_result( f0, f1, f2, f3, f4 );
#endif

    f0 = i8 == 16;
    f1 = i8 > 16;
    f2 = i8 >= 16;
    f3 = i8 < 16;
    f4 = i8 <= 16;
    show_result_bool( f0, f1, f2, f3, f4 );

    f0 = i16 == 32;
    f1 = i16 > 32;
    f2 = i16 >= 32;
    f3 = i16 < 32;
    f4 = i16 <= 32;
    show_result_bool( f0, f1, f2, f3, f4 );

    f0 = i32 == 64;
    f1 = i32 > 64;
    f2 = i32 >= 64;
    f3 = i32 < 64;
    f4 = i32 <= 64;
    show_result_bool( f0, f1, f2, f3, f4 );

#if 0
    f0 = i64 == 8;
    f1 = i64 > 8;
    f2 = i64 >= 8;
    f3 = i64 < 8;
    f4 = i64 <= 8;
    show_result_bool( f0, f1, f2, f3, f4 );
#endif

    printf( "testing printf\n" );

    printf( "  string: '%s'\n", "hello" );
    printf( "  char: '%c'\n", 'h' );
    printf( "  int: %d, %x\n", 27, 27 );
    printf( "  negative int: %d, %x\n", -27, -27 );

    printf( "stop\n" );
    return 0;
}

