#include <stdio.h>

/* this version of the C compiler doesn't support unsigned char or unsigned long

/*typedef unsigned char uint8_t; */
typedef char int8_t;
typedef unsigned short uint16_t;
typedef short int16_t;
/* typedef unsigned long uint32_t; */
typedef long int32_t;
typedef unsigned int uint;
typedef int bool;
#define false 0
#define true 1;

#define ab( x ) ( x < 0 ) ? ( -x ) : ( x )

int i8_test( i8A, i8B ) int8_t i8A; int8_t i8B;
{
    int8_t i8C;
    i8C = i8A * i8B;
    printf( "i8 %d * %d: %d\n", (int) i8A, (int) i8B, (int) i8C );
    i8C = i8A % i8B;
    printf( "i8 %d %% %d: %d\n", (int) i8A, (int) i8B, (int) i8C );
    i8C = i8A / i8B;
    printf( "i8 %d / %d: %d\n", (int) i8A, (int) i8B, (int) i8C );
}

int i16_test( i16A, i16B ) int16_t i16A; int16_t i16B;
{
    int16_t i16C;
    i16C = i16A * i16B;
    printf( "i16 %d * %d: %d\n", (int) i16A, (int) i16B, (int) i16C );
    i16C = i16A % i16B;
    printf( "i16 %d %% %d: %d\n", (int) i16A, (int) i16B, (int) i16C );
    i16C = i16A / i16B;
    printf( "i16 %d / %d: %d\n", (int) i16A, (int) i16B, (int) i16C );
}

int ui16_test( ui16A, ui16B ) uint16_t ui16A; uint16_t ui16B;
{
    uint16_t ui16C;
    ui16C = ui16A * ui16B;
    printf( "ui16 %u * %u: %u\n", (uint) ui16A, (uint) ui16B, (uint) ui16C );
    ui16C = ui16A % ui16B;
    printf( "ui16 %u %% %u: %u\n", (uint) ui16A, (uint) ui16B, (uint) ui16C );
    ui16C = ui16A / ui16B;
    printf( "ui16 %u / %u: %u\n", (uint) ui16A, (uint) ui16B, (uint) ui16C );
}

int i32_test( i32A, i32B ) int32_t i32A; int32_t i32B;
{
    int32_t i32C;
    i32C = i32A * i32B;
    printf( "i32 %ld * %ld: %ld\n", i32A, i32B, i32C );
    i32C = i32A % i32B;
    printf( "i32 %ld %% %ld: %ld\n", i32A, i32B, i32C );
    i32C = i32A / i32B;
    printf( "i32 %ld / %ld: %ld\n", i32A, i32B, i32C );
}

int main()
{
    printf( "app start\n" );
    fflush( stdout );

    i8_test( (int8_t) 3, (int8_t) 14 );
    i8_test( (int8_t) 17, (int8_t) 14 );
    i8_test( (int8_t) -3, (int8_t) 14 );
    i8_test( (int8_t) -17, (int8_t) 14 );
    i8_test( (int8_t) -3, (int8_t) -14 );
    i8_test( (int8_t) -17, (int8_t) -14 );
    i8_test( (int8_t) 28, (int8_t) 4 );
    i8_test( (int8_t) 28, (int8_t) -4 );
    i8_test( (int8_t) -28, (int8_t) 4 );
    i8_test( (int8_t) -28, (int8_t) -4 );

    i16_test( (int16_t) 3, (int16_t) 14 );
    i16_test( (int16_t) 3700, (int16_t) 14 );
    i16_test( (int16_t) -3, (int16_t) 14 );
    i16_test( (int16_t) -3700, (int16_t) 14 );
    i16_test( (int16_t) -3, (int16_t) -14 );
    i16_test( (int16_t) -3700, (int16_t) -14 );
    i16_test( (int16_t) 2800, (int16_t) 4 );
    i16_test( (int16_t) 2800, (int16_t) -4 );
    i16_test( (int16_t) -2800, (int16_t) 4 );
    i16_test( (int16_t) -2800, (int16_t) -4 );

    ui16_test( (uint16_t) 3, (uint16_t) 14 );
    ui16_test( (uint16_t) 3700, (uint16_t) 14 );
    ui16_test( (uint16_t) -3, (uint16_t) 14 );
    ui16_test( (uint16_t) -3700, (uint16_t) 14 );
    ui16_test( (uint16_t) -3, (uint16_t) -14 );
    ui16_test( (uint16_t) -3700, (uint16_t) -14 );
    ui16_test( (uint16_t) 2800, (uint16_t) 4 );
    ui16_test( (uint16_t) 2800, (uint16_t) -4 );
    ui16_test( (uint16_t) -2800, (uint16_t) 4 );
    ui16_test( (uint16_t) -2800, (uint16_t) -4 );

    i32_test( (int32_t) 3, (int32_t) 14 );
    i32_test( (int32_t) 37000L, (int32_t) 14 );
    i32_test( (int32_t) -3, (int32_t) 14 );
    i32_test( (int32_t) -37000L, (int32_t) 14 );
    i32_test( (int32_t) -3, (int32_t) -14 );
    i32_test( (int32_t) -37000L, (int32_t) -14 );
    i32_test( (int32_t) 280000, (int32_t) 4 );
    i32_test( (int32_t) 280000, (int32_t) -4 );
    i32_test( (int32_t) -280000, (int32_t) 4 );
    i32_test( (int32_t) -280000, (int32_t) -4 );

    printf( "tmuldiv ended with great success\n" );
    fflush( stdout );
    return 0;
} 

