#include <stdio.h>

typedef unsigned char uint8_t;
typedef char int8_t; /* DR compiler fails with a syntax error when using "signed char" */
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned long uint32_t;
typedef long int32_t;
typedef int bool;
#define true 1
#define false 0

float atof();

void sr_i8( x ) int8_t x; { printf( "  sizeof int8_t: %d, result %x\n", sizeof( x ), (uint8_t) x ); }
void sr_ui8( x ) uint8_t x; { printf( "  sizeof uint8_t: %d, result %x\n", sizeof( x ), x ); }
void sr_i16( x ) int16_t x; { printf( "  sizeof int16_t: %d, result %x\n", sizeof( x ), x ); }
void sr_ui16( x ) uint16_t x; { printf( "  sizeof uint16_t: %d, result %x\n", sizeof( x ), x ); }
void sr_i32( x ) int32_t x; { printf( "  sizeof int32_t: %d, result %lx\n", sizeof( x ), x ); }
void sr_ui32( x ) uint32_t x; { printf( "  sizeof uint32_t: %d, result %lx\n", sizeof( x ), x ); }

char bc( x ) bool x; { return x ? 't' : 'f'; }

void sr_bool( a, b, c, d, e ) bool a; bool b; bool c; bool d; bool e;
{
    printf( "  %c, %c, %c, %c, %c\n", bc( a ), bc( b ), bc( c ), bc( d ), bc( e ) );
}

int main()
{
    int8_t i8;
    uint8_t ui8;
    int16_t i16;
    uint16_t ui16;
    int32_t i32;
    uint32_t ui32;
    bool f0, f1, f2, f3, f4;

    printf( "test right shifts\n" );

/*
    The DR C compiler treats i8 as a word for the shift and just gets lucky that the result in the high byte is the same as the low byte
        pc     7d3a  op  1d7c fffffffe tscvznx a0:c326 a2:7a81 a5:7a81 a6:a2c2ae a7:a2c290 ==> move.b #$ffffffff, -2(a6)
        pc     7d40  op  102e fffe4880 tscvzNx a0:c326 a2:7a81 a5:7a81 a6:a2c2ae a7:a2c290 ==> move.b -2(a6), d0
        pc     7d44  op  4880 e2401d40 tscvzNx d0:ff a0:c326 a2:7a81 a5:7a81 a6:a2c2ae a7:a2c290 ==> ext.w d0
        pc     7d46  op  e240 1d40fffe tscvzNx d0:ffff a0:c326 a2:7a81 a5:7a81 a6:a2c2ae a7:a2c290 ==> asr.w #1, d0
        pc     7d48  op  1d40 fffe102e tsCvzNX d0:ffff a0:c326 a2:7a81 a5:7a81 a6:a2c2ae a7:a2c290 ==> move.b d0, -2(a6)
        pc     7d4c  op  102e fffe4880 tscvzNX d0:ffff a0:c326 a2:7a81 a5:7a81 a6:a2c2ae a7:a2c290 ==> move.b -2(a6), d0
*/

    i8 = -1;
    i8 >>= 1;
    sr_i8( i8 );

/*
    The DR C compiler has a bug with unsigned char right shifts. It treats the value as 16 bits for the shift and grabs the low byte:
        pc     7d3a  op  1d7c   fffffc tscvznx a0:c308 a2:7a81 a5:7a81 a6:a2c28e a7:a2c270 ==> move.b #$ff, -4(a6)
        pc     7d40  op  102e fffc4880 tscvzNx a0:c308 a2:7a81 a5:7a81 a6:a2c28e a7:a2c270 ==> move.b -4(a6), d0
        pc     7d44  op  4880 e2481d40 tscvzNx d0:ff a0:c308 a2:7a81 a5:7a81 a6:a2c28e a7:a2c270 ==> ext.w d0
        pc     7d46  op  e248 1d40fffc tscvzNx d0:ffff a0:c308 a2:7a81 a5:7a81 a6:a2c28e a7:a2c270 ==> lsr.w #1, d0
        pc     7d48  op  1d40 fffc4240 tsCvznX d0:7fff a0:c308 a2:7a81 a5:7a81 a6:a2c28e a7:a2c270 ==> move.b d0, -4(a6)
*/

    ui8 = 0xff;
    ui8 >>= 1;
    sr_ui8( ui8 );

    i16 = -1;
    i16 >>= 1;
    sr_i16( i16 );

    ui16 = 0xffff;
    ui16 >>= 1;
    sr_ui16( ui16 );

    i32 = -1;
    i32 >>= 1;
    sr_i32( i32 );

    ui32 = 0xffffffff;
    ui32 >>= 1;
    sr_ui32( ui32 );

    printf( "now test left shifts\n" );

    i8 = -1;
    i8 <<= 1;
    sr_i8( i8 );

    ui8 = 0xff;
    ui8 <<= 1;
    sr_ui8( ui8 );

    i16 = -1;
    i16 <<= 1;
    sr_i16( i16 );

    ui16 = 0xffff;
    ui16 <<= 1;
    sr_ui16( ui16 );

    i32 = -1;
    i32 <<= 1;
    sr_i32( i32 );

    ui32 = 0xffffffff;
    ui32 <<= 1;
    sr_ui32( ui32 );

    printf( "now test comparisons. t, f, t, f, t expected\n" );

    f0 = i8 == ui8;
    f1 = i8 > ui8;
    f2 = i8 >= ui8;
    f3 = i8 < ui8;
    f4 = i8 <= ui8;
    sr_bool( f0, f1, f2, f3, f4 );

    f0 = i16 == ui16;
    f1 = i16 > ui16;
    f2 = i16 >= ui16;
    f3 = i16 < ui16;
    f4 = i16 <= ui16;
    sr_bool( f0, f1, f2, f3, f4 );

    f0 = i32 == ui32;
    f1 = i32 > ui32;
    f2 = i32 >= ui32;
    f3 = i32 < ui32;
    f4 = i32 <= ui32;
    sr_bool( f0, f1, f2, f3, f4 );

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

    printf( "testing printf\n" );

    printf( "  string: '%s'\n", "hello" );
    printf( "  char: '%c'\n", 'h' );
    printf( "  int: %d, %x\n", 27, 27 );
    printf( "  negative int: %d, %x\n", -27, -27 );
    printf( "  float: %f\n", atof( "3.1415729" ) );
    printf( "  negative float: %f\n", atof( "-3.1415729" ) );

    printf( "ts completed with great success\n" );
    return 0;
} 


