#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

template <class T> void test_bits( T a, T b )
{
    T r_and = ( a & b );
    T r_or = ( a | b );
    T r_xor = ( a ^ b );
    T r_nota = ( ~a );
    T r_notb = ( ~b );

//    if ( sizeof( a ) == 8 )
        printf( "  and %llx, or %llx, xor %llx, nota %llx, notb %llx\n",
                (uint64_t) r_and, (uint64_t) r_or, (uint64_t) r_xor, (uint64_t) r_nota, (uint64_t) r_notb );
//    else
//        printf( "  and %x, or %x, xor %x, nota %x, notb %x\n",
//                r_and, r_or, r_xor, r_nota, r_notb );
} //test_bits

int main( int argc, char * argv[] )
{
    printf( "uint8_t:\n" );
    test_bits( (uint8_t) 7, (uint8_t) 3 );
    test_bits( (uint8_t) 7, (uint8_t) -3 );
    test_bits( (uint8_t) -7, (uint8_t) 3 );
    test_bits( (uint8_t) -7, (uint8_t) -3 );
    test_bits( (uint8_t) -1, (uint8_t) -1 );
    test_bits( (uint8_t) 247, (uint8_t) 3 );
    test_bits( (uint8_t) 247, (uint8_t) -3 );
    test_bits( (uint8_t) -247, (uint8_t) 3 );
    test_bits( (uint8_t) -247, (uint8_t) -247 );

    printf( "int8_t:\n" );
    test_bits( (int8_t) 7, (int8_t) 3 );
    test_bits( (int8_t) 7, (int8_t) -3 );
    test_bits( (int8_t) -7, (int8_t) 3 );
    test_bits( (int8_t) -7, (int8_t) -3 );
    test_bits( (int8_t) -1, (int8_t) -1 );
    test_bits( (int8_t) 247, (int8_t) 3 );
    test_bits( (int8_t) 247, (int8_t) -3 );
    test_bits( (int8_t) -247, (int8_t) 3 );
    test_bits( (int8_t) -247, (int8_t) -247 );

    printf( "uint16_t:\n" );
    test_bits( (uint16_t) 7, (uint16_t) 3 );
    test_bits( (uint16_t) 7, (uint16_t) -3 );
    test_bits( (uint16_t) -7, (uint16_t) 3 );
    test_bits( (uint16_t) -7, (uint16_t) -3 );
    test_bits( (uint16_t) -1, (uint16_t) -1 );
    test_bits( (uint16_t) 247, (uint16_t) 3 );
    test_bits( (uint16_t) 247, (uint16_t) -3 );
    test_bits( (uint16_t) -247, (uint16_t) 3 );
    test_bits( (uint16_t) -247, (uint16_t) -247 );

    printf( "int16_t:\n" );
    test_bits( (int16_t) 7, (int16_t) 3 );
    test_bits( (int16_t) 7, (int16_t) -3 );
    test_bits( (int16_t) -7, (int16_t) 3 );
    test_bits( (int16_t) -7, (int16_t) -3 );
    test_bits( (int16_t) -1, (int16_t) -1 );
    test_bits( (int16_t) 247, (int16_t) 3 );
    test_bits( (int16_t) 247, (int16_t) -3 );
    test_bits( (int16_t) -247, (int16_t) 3 );
    test_bits( (int16_t) -247, (int16_t) -247 );

    printf( "uint32_t:\n" );
    test_bits( (uint32_t) 7, (uint32_t) 3 );
    test_bits( (uint32_t) 7, (uint32_t) -3 );
    test_bits( (uint32_t) -7, (uint32_t) 3 );
    test_bits( (uint32_t) -7, (uint32_t) -3 );
    test_bits( (uint32_t) -1, (uint32_t) -1 );
    test_bits( (uint32_t) 247, (uint32_t) 3 );
    test_bits( (uint32_t) 247, (uint32_t) -3 );
    test_bits( (uint32_t) -247, (uint32_t) 3 );
    test_bits( (uint32_t) -247, (uint32_t) -247 );

    printf( "int32_t:\n" );
    test_bits( (int32_t) 7, (int32_t) 3 );
    test_bits( (int32_t) 7, (int32_t) -3 );
    test_bits( (int32_t) -7, (int32_t) 3 );
    test_bits( (int32_t) -7, (int32_t) -3 );
    test_bits( (int32_t) -1, (int32_t) -1 );
    test_bits( (int32_t) 247, (int32_t) 3 );
    test_bits( (int32_t) 247, (int32_t) -3 );
    test_bits( (int32_t) -247, (int32_t) 3 );
    test_bits( (int32_t) -247, (int32_t) -247 );

    printf( "uint64_t:\n" );
    test_bits( (uint64_t) 7, (uint64_t) 3 );
    test_bits( (uint64_t) 7, (uint64_t) -3 );
    test_bits( (uint64_t) -7, (uint64_t) 3 );
    test_bits( (uint64_t) -7, (uint64_t) -3 );
    test_bits( (uint64_t) -1, (uint64_t) -1 );
    test_bits( (uint64_t) 247, (uint64_t) 3 );
    test_bits( (uint64_t) 247, (uint64_t) -3 );
    test_bits( (uint64_t) -247, (uint64_t) 3 );
    test_bits( (uint64_t) -247, (uint64_t) -247 );

    printf( "int64_t:\n" );
    test_bits( (int64_t) 7, (int64_t) 3 );
    test_bits( (int64_t) 7, (int64_t) -3 );
    test_bits( (int64_t) -7, (int64_t) 3 );
    test_bits( (int64_t) -7, (int64_t) -3 );
    test_bits( (int64_t) -1, (int64_t) -1 );
    test_bits( (int64_t) 247, (int64_t) 3 );
    test_bits( (int64_t) 247, (int64_t) -3 );
    test_bits( (int64_t) -247, (int64_t) 3 );
    test_bits( (int64_t) -247, (int64_t) -247 );
} //main

