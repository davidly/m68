#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void swap( char & a, char & b )
{
    char c = a;
    a = b;
    b = c;
} //swap

void reverse( char str[], int length )
{
    int start = 0;
    int end = length - 1;
    while ( start < end )
    {
        swap( * ( str + start ), * ( str + end ) );
        start++;
        end--;
    }
} //reverse

char * ui64toa( uint64_t num, char * str, int base )
{
    int i = 0;
    bool isNegative = false;

    if ( 0 == num )
    {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    if ( num < 0 && 10 == base )
    {
        isNegative = true;
        num = -num;
    }

    while ( 0 != num )
    {
        int rem = num % base;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/base;
    }

    if (isNegative)
        str[i++] = '-';

    str[i] = '\0';

    reverse( str, i );

    return str;
} //ui64toa

char * ui32toa( uint32_t num, char * str, int base )
{
    int i = 0;
    bool isNegative = false;

    if ( 0 == num )
    {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    if ( num < 0 && 10 == base )
    {
        isNegative = true;
        num = -num;
    }

    while ( 0 != num )
    {
        int rem = num % base;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/base;
    }

    if (isNegative)
        str[i++] = '-';

    str[i] = '\0';

    reverse( str, i );

    return str;
} //ui32toa

template <class T> T myabs( T x )
{
    if ( x < 0 )
        return -x;
    return x;
} //myabs

#if defined( _MSC_VER ) || defined(WIN64)

extern "C" void riscv_print_text( const char * p )
{
    printf( "%s", p );
}
#endif

template <class T> void show_result( T x )
{
    if ( sizeof( T ) <= 4 )
        printf( "    sizeof T: %d, result: %x\n", (int) sizeof( T ), (uint32_t) x );
    else
        printf( "    sizeof T: %d, result: %llx\n", (int) sizeof( T ), x );
} //show_result

char bc( bool x ) { return x ? 't' : 'f'; }
void sr_bool( bool a, bool b, bool c, bool d, bool e )
{
    printf( "    %c, %c, %c, %c, %c\n", bc( a ), bc( b ), bc( c ), bc( d ), bc( e ) );
}

//#pragma GCC optimize ("O0")
extern "C" int main()
{
    printf( "test right shifts\n" );

    int8_t i8 = -1;
    i8 >>= 1;
    show_result( (uint8_t) i8 );

    uint8_t ui8 = 0xff;
    ui8 >>= 1;
    show_result( ui8 );

    int16_t i16 = -1;
    i16 >>= 1;
    show_result( (uint16_t) i16 );

    uint16_t ui16 = 0xffff;
    ui16 >>= 1;
    show_result( ui16 );

    int32_t i32 = -1;
    i32 >>= 1;
    show_result( (uint32_t) i32 );

    uint32_t ui32 = 0xffffffff;
    ui32 >>= 1;
    show_result( ui32 );

    int64_t i64 = -1;
    i64 >>= 1;
    show_result( i64 );

    uint64_t ui64 = 0xffffffffffffffff;
    ui64 >>= 1;
    show_result( ui64 );

    //////////////

    printf( "now test left shifts\n" );

    i8 = -1;
    i8 <<= 1;
    show_result( (uint8_t) i8 );

    ui8 = 0xff;
    ui8 <<= 1;
    show_result( ui8 );

    i16 = -1;
    i16 <<= 1;
    show_result( (uint16_t) i16 );

    ui16 = 0xffff;
    ui16 <<= 1;
    show_result( ui16 );

    i32 = -1;
    i32 <<= 1;
    show_result( (uint32_t) i32 );

    ui32 = 0xffffffff;
    ui32 <<= 1;
    show_result( ui32 );

    i64 = -1;
    i64 <<= 1;
    show_result( i64 );

    ui64 = 0xffffffffffffffff;
    ui64 <<= 1;
    show_result( ui64 );

    //////////////

    printf( "now test comparisons. f, f, f, t, t expected\n" );

    bool f0 = i8 == ui8;
    bool f1 = i8 > ui8;
    bool f2 = i8 >= ui8;
    bool f3 = i8 < ui8;
    bool f4 = i8 <= ui8;
    sr_bool( f0, f1, f2, f3, f4 );

    f0 = i16 == ui16;
    f1 = i16 > ui16;
    f2 = i16 >= ui16;
    f3 = i16 < ui16;
    f4 = i16 <= ui16;
    sr_bool( f0, f1, f2, f3, f4 );

    printf( "more test comparisons. t, f, t, f, t expected\n" );
    f0 = i32 == ui32;
    f1 = i32 > ui32;
    f2 = i32 >= ui32;
    f3 = i32 < ui32;
    f4 = i32 <= ui32;
    sr_bool( f0, f1, f2, f3, f4 );

    f0 = i64 == ui64;
    f1 = i64 > ui64;
    f2 = i64 >= ui64;
    f3 = i64 < ui64;
    f4 = i64 <= ui64;
    sr_bool( f0, f1, f2, f3, f4 );

    //////////////

    f0 = i8 == i16;
    f1 = i8 > i16;
    f2 = i8 >= i16;
    f3 = i8 < i16;
    f4 = i8 <= i16;
    sr_bool( f0, f1, f2, f3, f4 );

    f0 = i16 == i32;
    f1 = i16 > i32;
    f2 = i16 >= i32;
    f3 = i16 < i32;
    f4 = i16 <= i32;
    sr_bool( f0, f1, f2, f3, f4 );

    f0 = i32 == i64;
    f1 = i32 > i64;
    f2 = i32 >= i64;
    f3 = i32 < i64;
    f4 = i32 <= i64;
    sr_bool( f0, f1, f2, f3, f4 );

    printf( "more test comparisons. f, f, f, t, t expected\n" );
    f0 = i64 == ui8;
    f1 = i64 > ui8;
    f2 = i64 >= ui8;
    f3 = i64 < ui8;
    f4 = i64 <= ui8;
    sr_bool( f0, f1, f2, f3, f4 );

    //////////////

    printf( "more comparisons. f, f, f, t, t expected\n" );

    f0 = i8 == 16;
    f1 = i8 > 16;
    f2 = i8 >= 16;
    f3 = i8 < 16;
    f4 = i8 <= 16;
    sr_bool( f0, f1, f2, f3, f4 );

    f0 = i16 == 32;
    f1 = i16 > 32;
    f2 = i16 >= 32;
    f3 = i16 < 32;
    f4 = i16 <= 32;
    sr_bool( f0, f1, f2, f3, f4 );

    f0 = i32 == 64;
    f1 = i32 > 64;
    f2 = i32 >= 64;
    f3 = i32 < 64;
    f4 = i32 <= 64;
    sr_bool( f0, f1, f2, f3, f4 );

    f0 = i64 == 8;
    f1 = i64 > 8;
    f2 = i64 >= 8;
    f3 = i64 < 8;
    f4 = i64 <= 8;
    sr_bool( f0, f1, f2, f3, f4 );

    //////////////////

    printf( "testing printf\n" );

    printf( "  string: '%s'\n", "hello" );
    printf( "  char: '%c'\n", 'h' );
    printf( "  int: %d, %x\n", 27, 27 );
    printf( "  negative int: %d, %x\n", -27, -27 );
    printf( "  int64_t: %lld, %llx\n", (int64_t) 27, (int64_t) 27 );
    printf( "  negative int64_t: %lld, %llx\n", (int64_t) -27, (int64_t) -27 );
    printf( "  float: %f\n", 3.1415729 );
    printf( "  negative float: %f\n", -3.1415729 );

    printf( "stop\n" );
    return 0;
} //main
