#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#if 0
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned long uint32_t;
typedef signed long int32_t;
typedef unsigned int uint;
typedef int bool;
enum { false, true };
#endif

typedef unsigned int uint;

#define _noinline __attribute__((noinline))

#define ab( x ) ( x < 0 ) ? ( -x ) : ( x )

_noinline void i8_test( int8_t i8A, int8_t i8B )
{
    int8_t i8C = i8A * i8B;
    printf( "i8 %d * %d: %d\n", (int) i8A, (int) i8B, (int) i8C );
    i8C = i8A % i8B;
    printf( "i8 %d %% %d: %d\n", (int) i8A, (int) i8B, (int) i8C );
    i8C = i8A / i8B;
    printf( "i8 %d / %d: %d\n", (int) i8A, (int) i8B, (int) i8C );
}

_noinline void ui8_test( uint8_t ui8A, uint8_t ui8B )
{
    uint8_t ui8C = ui8A * ui8B;
    printf( "ui8 %u * %u: %u\n", (uint) ui8A, (uint) ui8B, (uint) ui8C );
    ui8C = ui8A % ui8B;
    printf( "ui8 %u %% %u: %u\n", (uint) ui8A, (uint) ui8B, (uint) ui8C );
    ui8C = ui8A / ui8B;
    printf( "ui8 %u / %u: %u\n", (uint) ui8A, (uint) ui8B, (uint) ui8C );
}

_noinline void i16_test( int16_t i16A, int16_t i16B )
{
    int16_t i16C = i16A * i16B;
    printf( "i16 %d * %d: %d\n", (int) i16A, (int) i16B, (int) i16C );
    i16C = i16A % i16B;
    printf( "i16 %d %% %d: %d\n", (int) i16A, (int) i16B, (int) i16C );
    i16C = i16A / i16B;
    printf( "i16 %d / %d: %d\n", (int) i16A, (int) i16B, (int) i16C );
}

_noinline void ui16_test( uint16_t ui16A, uint16_t ui16B )
{
    uint16_t ui16C = ui16A * ui16B;
    printf( "ui16 %u * %u: %u\n", (uint) ui16A, (uint) ui16B, (uint) ui16C );
    ui16C = ui16A % ui16B;
    printf( "ui16 %u %% %u: %u\n", (uint) ui16A, (uint) ui16B, (uint) ui16C );
    ui16C = ui16A / ui16B;
    printf( "ui16 %u / %u: %u\n", (uint) ui16A, (uint) ui16B, (uint) ui16C );
}

_noinline void i32_test( int32_t i32A, int32_t i32B )
{
    int32_t i32C = i32A * i32B;
    printf( "i32 %d * %d: %d\n", i32A, i32B, i32C );
    i32C = i32A % i32B;
    printf( "i32 %d %% %d: %d\n", i32A, i32B, i32C );
    i32C = i32A / i32B;
    printf( "i32 %d / %d: %d\n", i32A, i32B, i32C );
}

_noinline void ui32_test( uint32_t ui32A, uint32_t ui32B )
{
    uint32_t ui32C = ui32A * ui32B;
    printf( "ui32 %u * %u: %u\n", ui32A, ui32B, ui32C );
    ui32C = ui32A % ui32B;
    printf( "ui32 %u %% %u: %u\n", ui32A, ui32B, ui32C );
    ui32C = ui32A / ui32B;
    printf( "ui32 %u / %u: %u\n", ui32A, ui32B, ui32C );
}

_noinline void i64_test( int64_t i64A, int64_t i64B )
{
    int64_t i64C = i64A * i64B;
    printf( "i64 %lld * %lld: %lld\n", i64A, i64B, i64C );
    i64C = i64A % i64B;
    printf( "i64 %lld %% %lld: %lld\n", i64A, i64B, i64C );
    i64C = i64A / i64B;
    printf( "i64 %lld / %lld: %lld\n", i64A, i64B, i64C );
}

_noinline void ui64_test( uint64_t ui64A, uint64_t ui64B )
{
    uint64_t ui64C = ui64A * ui64B;
    printf( "ui64 %llu * %llu: %llu\n", ui64A, ui64B, ui64C );
    ui64C = ui64A % ui64B;
    printf( "ui64 %llu %% %llu: %llu\n", ui64A, ui64B, ui64C );
    ui64C = ui64A / ui64B;
    printf( "ui64 %llu / %llu: %llu\n", ui64A, ui64B, ui64C );
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

    ui8_test( (uint8_t) 3, (uint8_t) 14 );
    ui8_test( (uint8_t) 17, (uint8_t) 14 );
    ui8_test( (uint8_t) -3, (uint8_t) 14 );
    ui8_test( (uint8_t) -17, (uint8_t) 14 );
    ui8_test( (uint8_t) -3, (uint8_t) -14 );
    ui8_test( (uint8_t) -17, (uint8_t) -14 );
    ui8_test( (uint8_t) 28, (uint8_t) 4 );
    ui8_test( (uint8_t) 28, (uint8_t) -4 );
    ui8_test( (uint8_t) -28, (uint8_t) 4 );
    ui8_test( (uint8_t) -28, (uint8_t) -4 );

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
    i32_test( (int32_t) 37000, (int32_t) 14 );
    i32_test( (int32_t) -3, (int32_t) 14 );
    i32_test( (int32_t) -37000, (int32_t) 14 );
    i32_test( (int32_t) -3, (int32_t) -14 );
    i32_test( (int32_t) -37000, (int32_t) -14 );
    i32_test( (int32_t) 280000, (int32_t) 4 );
    i32_test( (int32_t) 280000, (int32_t) -4 );
    i32_test( (int32_t) -280000, (int32_t) 4 );
    i32_test( (int32_t) -280000, (int32_t) -4 );

    ui32_test( (uint32_t) 3, (uint32_t) 14 );
    ui32_test( (uint32_t) 37000, (uint32_t) 14 );
    ui32_test( (uint32_t) -3, (uint32_t) 14 );
    ui32_test( (uint32_t) -37000, (uint32_t) 14 );
    ui32_test( (uint32_t) -3, (uint32_t) -14 );
    ui32_test( (uint32_t) -37000, (uint32_t) -14 );
    ui32_test( (uint32_t) 280000, (uint32_t) 4 );
    ui32_test( (uint32_t) 280000, (uint32_t) -4L );
    ui32_test( (uint32_t) -280000, (uint32_t) 4 );
    ui32_test( (uint32_t) -280000, (uint32_t) -4L );

    i64_test( (int64_t) 3, (int64_t) 14 );
    i64_test( (int64_t) 370000000, (int64_t) 14 );
    i64_test( (int64_t) -3, (int64_t) 14 );
    i64_test( (int64_t) -370000000, (int64_t) 14 );
    i64_test( (int64_t) -3, (int64_t) -14 );
    i64_test( (int64_t) -370000000, (int64_t) -14 );
    i64_test( (int64_t) 28000000000000, (int64_t) 4 );
    i64_test( (int64_t) 28000000000000, (int64_t) -4 );
    i64_test( (int64_t) -28000000000000, (int64_t) 4 );
    i64_test( (int64_t) -28000000000000, (int64_t) -4 );

    ui64_test( (uint64_t) 3, (uint64_t) 14 );
    ui64_test( (uint64_t) 370000000, (uint64_t) 14 );
    ui64_test( (uint64_t) -3, (uint64_t) 14 );
    ui64_test( (uint64_t) -370000000, (uint64_t) 14 );
    ui64_test( (uint64_t) -3, (uint64_t) -14 );
    ui64_test( (uint64_t) -370000000, (uint64_t) -14 );
    ui64_test( (uint64_t) 28000000000000, (uint64_t) 4 );
    ui64_test( (uint64_t) 28000000000000, (uint64_t) -4L );
    ui64_test( (uint64_t) -28000000000000, (uint64_t) 4 );
    ui64_test( (uint64_t) -28000000000000, (uint64_t) -4L );

    printf( "tmuldiv ended with great success\n" );
    fflush( stdout );
    return 0;
}

