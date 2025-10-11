/* test array operations. */

#define LINT_ARGS

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <cmath>

#define _perhaps_inline __attribute__((noinline))
//#define _perhaps_inline

template <class T> T do_abs( T x )
{
    return ( x < 0 ) ? -x : x;
}

#define array_operations_test( ftype, dim ) \
    ftype A_##ftype##dim[ dim ]; \
    ftype B_##ftype##dim[ dim ]; \
    ftype C_##ftype##dim[ dim ]; \
    ftype D_##ftype##dim[ dim ]; \
    _perhaps_inline void fillA_##ftype##dim( ftype val ) \
    { \
        for ( int i = 0; i < dim; i++ ) \
            A_##ftype##dim[ i ] = val + i; \
    } \
    _perhaps_inline void fillB_##ftype##dim( ftype val ) \
    { \
        for ( int i = 0; i < dim; i++ ) \
            B_##ftype##dim[ i ] = val + i; \
    } \
    _perhaps_inline void fillCD_##ftype##dim( ftype val ) \
    { \
        for ( int i = 0; i < dim; i++ ) \
        { \
            C_##ftype##dim[ i ] = val + i * 2; \
            D_##ftype##dim[ i ] = val + i * 2; \
        } \
    } \
    _perhaps_inline void randomizeB_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            B_##ftype##dim[ i ] = rand(); \
    } \
    _perhaps_inline void randomizeCD_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            D_##ftype##dim[ i ] = C_##ftype##dim[ i ] = rand(); \
    } \
    _perhaps_inline void shift_left_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            A_##ftype##dim[ i ] <<= ( 1 + i ); \
    } \
    _perhaps_inline void shift_left_n_##ftype##dim() \
    { \
        fillB_##ftype##dim( 1 ); \
        for ( int i = 0; i < dim; i++ ) \
            A_##ftype##dim[ i ] <<= B_##ftype##dim[ i ]; \
    } \
    _perhaps_inline void shift_right_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            A_##ftype##dim[ i ] >>= ( 1 + i ); \
    } \
    _perhaps_inline void shift_right_n_##ftype##dim() \
    { \
        fillB_##ftype##dim( 1 ); \
        for ( int i = 0; i < dim; i++ ) \
            A_##ftype##dim[ i ] >>= B_##ftype##dim[ i ]; \
    } \
    _perhaps_inline void and_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            A_##ftype##dim[ i ] &= ( i + ~0x33 ); \
    } \
    _perhaps_inline void and_n_##ftype##dim() \
    { \
        fillB_##ftype##dim( ~0x33 ); \
        for ( int i = 0; i < dim; i++ ) \
            A_##ftype##dim[ i ] &= B_##ftype##dim[ i ]; \
    } \
    _perhaps_inline void or_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            A_##ftype##dim[ i ] |= ( i + 0x55 ); \
    } \
    _perhaps_inline void or_n_##ftype##dim() \
    { \
        fillB_##ftype##dim( 0x55 ); \
        for ( int i = 0; i < dim; i++ ) \
            A_##ftype##dim[ i ] |= B_##ftype##dim[ i ]; \
    } \
    _perhaps_inline void eor_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            A_##ftype##dim[ i ] ^= ( i + 0x99 ); \
    } \
    _perhaps_inline void eor_n_##ftype##dim() \
    { \
        fillB_##ftype##dim( 0x99 ); \
        for ( int i = 0; i < dim; i++ ) \
            A_##ftype##dim[ i ] ^= B_##ftype##dim[ i ]; \
    } \
    _perhaps_inline void add_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            A_##ftype##dim[ i ] += ( i + 2 ); \
    } \
    _perhaps_inline void add_n_##ftype##dim() \
    { \
        fillB_##ftype##dim( 2 ); \
        for ( int i = 0; i < dim; i++ ) \
            A_##ftype##dim[ i ] += B_##ftype##dim[ i ]; \
    } \
    _perhaps_inline void sub_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            A_##ftype##dim[ i ] -= ( i + 2 ); \
    } \
    _perhaps_inline void sub_n_##ftype##dim() \
    { \
        fillB_##ftype##dim( 2 ); \
        for ( int i = 0; i < dim; i++ ) \
            A_##ftype##dim[ i ] -= B_##ftype##dim[ i ]; \
    } \
    _perhaps_inline void mul_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            A_##ftype##dim[ i ] *= ( i + 2 ); \
    } \
    _perhaps_inline void mul_n_##ftype##dim() \
    { \
        fillB_##ftype##dim( 2 ); \
        for ( int i = 0; i < dim; i++ ) \
            A_##ftype##dim[ i ] *= B_##ftype##dim[ i ]; \
    } \
    _perhaps_inline void div_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            A_##ftype##dim[ i ] /= ( i + 2 ); \
    } \
    _perhaps_inline void div_n_##ftype##dim() \
    { \
        fillB_##ftype##dim( 2 ); \
        for ( int i = 0; i < dim; i++ ) \
            A_##ftype##dim[ i ] /= B_##ftype##dim[ i ]; \
    } \
    _perhaps_inline void print_array_##ftype##dim() \
    { \
        printf( "array:" ); \
        for ( int i = 0; i < dim; i++ ) \
            printf( " %.0lf", (double) A_##ftype##dim[ i ] ); \
        printf( "\n" ); \
    } \
    _perhaps_inline void print_array_B_##ftype##dim() \
    { \
        printf( "arrayB:" ); \
        for ( int i = 0; i < dim; i++ ) \
            printf( " %.0lf", (double) B_##ftype##dim[ i ] ); \
        printf( "\n" ); \
    } \
    _perhaps_inline void print_array_C_##ftype##dim() \
    { \
        printf( "arrayC:" ); \
        for ( int i = 0; i < dim; i++ ) \
            printf( " %.0lf", (double) C_##ftype##dim[ i ] ); \
        printf( "\n" ); \
    } \
    _perhaps_inline ftype count_A_GE_B_##ftype##dim() \
    { \
        ftype result = (ftype) 0; \
        for ( int i = 0; i < dim; i++ ) \
            result += ( A_##ftype##dim[ i ] >= B_##ftype##dim[ i ] ); \
        return result; \
    } \
    _perhaps_inline ftype count_A_GT_B_##ftype##dim() \
    { \
        ftype result = (ftype) 0; \
        for ( int i = 0; i < dim; i++ ) \
            result += ( A_##ftype##dim[ i ] > B_##ftype##dim[ i ] ); \
        return result; \
    } \
    _perhaps_inline ftype count_A_EQ_B_##ftype##dim() \
    { \
        ftype result = (ftype) 0; \
        for ( int i = 0; i < dim; i++ ) \
            result += ( A_##ftype##dim[ i ] == B_##ftype##dim[ i ] ); \
        return result; \
    } \
    _perhaps_inline ftype count_A_LE_B_##ftype##dim() \
    { \
        ftype result = (ftype) 0; \
        for ( int i = 0; i < dim; i++ ) \
            result += ( A_##ftype##dim[ i ] <= B_##ftype##dim[ i ] ); \
        return result; \
    } \
    _perhaps_inline ftype count_A_LT_B_##ftype##dim() \
    { \
        ftype result = (ftype) 0; \
        for ( int i = 0; i < dim; i++ ) \
            result += ( A_##ftype##dim[ i ] < B_##ftype##dim[ i ] ); \
        return result; \
    } \
    _perhaps_inline ftype sum_##ftype##dim() \
    { \
        ftype result = (ftype) 0; \
        for ( int i = 0; i < dim; i++ ) \
            result += A_##ftype##dim[ i ]; \
        return result; \
    } \
    _perhaps_inline ftype sum_C_##ftype##dim() \
    { \
        ftype result = (ftype) 0; \
        for ( int i = 0; i < dim; i++ ) \
            result += C_##ftype##dim[ i ]; \
        return result; \
    } \
    _perhaps_inline ftype sum_D_##ftype##dim() \
    { \
        ftype result = (ftype) 0; \
        for ( int i = 0; i < dim; i++ ) \
            result += D_##ftype##dim[ i ]; \
        return result; \
    } \
    _perhaps_inline ftype magnitude_##ftype##dim() \
    { \
        ftype result = (ftype) 0; \
        for ( int i = 0; i < dim; i++ ) \
            result += do_abs( A_##ftype##dim[ i ] ); \
        return result; \
    } \
    _perhaps_inline ftype min_##ftype##dim() \
    { \
        ftype result = A_##ftype##dim[ 0 ]; \
        for ( int i = 0; i < dim; i++ ) \
            if ( A_##ftype##dim[ i ] < result ) \
                result = A_##ftype##dim[ i ]; \
        return result; \
    } \
    _perhaps_inline ftype max_##ftype##dim() \
    { \
        ftype result = A_##ftype##dim[ 0 ]; \
        for ( int i = 0; i < dim; i++ ) \
            result = A_##ftype##dim[ i ] > result ? A_##ftype##dim[ i ] : result; \
        return result; \
    } \
    _perhaps_inline void inc_C_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            C_##ftype##dim[ i ]++; \
    } \
    _perhaps_inline void dec_C_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            C_##ftype##dim[ i ]--; \
    } \
    _perhaps_inline void add_mul_ABC_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            C_##ftype##dim[ i ] += B_##ftype##dim[ i ] * A_##ftype##dim[ i ]; \
    } \
    _perhaps_inline void sub_mul_ABC_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            C_##ftype##dim[ i ] -= B_##ftype##dim[ i ] * A_##ftype##dim[ i ]; \
    } \
    _perhaps_inline void add_mul_div2_ABC_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            C_##ftype##dim[ i ] += ( B_##ftype##dim[ i ] / 4 ) * A_##ftype##dim[ i ]; \
    } \
    _perhaps_inline void sub_mul_div2_ABC_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            C_##ftype##dim[ i ] -= ( B_##ftype##dim[ i ] / 4 ) * A_##ftype##dim[ i ]; \
    } \
    _perhaps_inline void add_mul_div2b_ABC_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            C_##ftype##dim[ i ] += ( B_##ftype##dim[ i ] * 4 ) * A_##ftype##dim[ i ]; \
    } \
    _perhaps_inline void sub_mul_div2b_ABC_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            C_##ftype##dim[ i ] -= ( B_##ftype##dim[ i ] * 4 ) * A_##ftype##dim[ i ]; \
    } \
    _perhaps_inline void add_div_ABC_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
        { \
            if ( 0 == A_##ftype##dim[ i ] ) \
                continue; \
            C_##ftype##dim[ i ] += B_##ftype##dim[ i ] / A_##ftype##dim[ i ]; \
            /*printf( "i: %d, C %d, A %d B %d\n", i, C_##ftype##dim[i], A_##ftype##dim[ i ], B_##ftype##dim[ i ] );*/ \
        } \
    } \
    _perhaps_inline void sub_div_ABC_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
        { \
            if ( 0 == A_##ftype##dim[ i ] ) \
                continue; \
            C_##ftype##dim[ i ] -= B_##ftype##dim[ i ] / A_##ftype##dim[ i ]; \
        } \
    } \
    ftype run_##ftype##dim() \
    { \
        fillA_##ftype##dim( -10 ); \
        /* printf( "initial: " ); print_array_##ftype##dim(); */ \
        shift_left_##ftype##dim(); \
        /* printf( "left: " ); print_array_##ftype##dim(); */ \
        shift_right_##ftype##dim(); \
        /* printf( "right: " ); print_array_##ftype##dim(); */ \
        and_##ftype##dim(); \
        /* printf( "and: " ); print_array_##ftype##dim(); */ \
        or_##ftype##dim(); \
        /* printf( "or: " ); print_array_##ftype##dim(); */ \
        eor_##ftype##dim(); \
        /* printf( "eor: " ); print_array_##ftype##dim(); */ \
        add_##ftype##dim(); \
        /* printf( "add: " ); print_array_##ftype##dim(); */ \
        sub_##ftype##dim(); \
        /* printf( "sub: " ); print_array_##ftype##dim(); */ \
        mul_##ftype##dim(); \
        /* printf( "eor: " ); print_array_##ftype##dim(); */ \
        div_##ftype##dim(); \
        /* printf( "eor: " ); print_array_##ftype##dim(); */ \
        ftype sum = sum_##ftype##dim(); \
        ftype magnitude = magnitude_##ftype##dim(); \
        printf( "type %s size %d, sum %.0lf, magnitude %.0lf min, %.0lf max %.0lf\n", #ftype, dim, (double) sum, (double) magnitude, \
                (double) min_##ftype##dim(), (double) max_##ftype##dim() ); \
        fillA_##ftype##dim( -10 ); \
        /* printf( "initial_n: " ); print_array_##ftype##dim(); */ \
        shift_left_n_##ftype##dim(); \
        /* printf( "left n: " ); print_array_##ftype##dim(); */  \
        shift_right_n_##ftype##dim(); \
        /* printf( "right_n: " ); print_array_##ftype##dim(); */  \
        and_n_##ftype##dim(); \
        /* printf( "and_n: " ); print_array_##ftype##dim(); */  \
        or_n_##ftype##dim(); \
        /* printf( "or_n: " ); print_array_##ftype##dim(); */  \
        eor_n_##ftype##dim(); \
        /* printf( "eor_n: " ); print_array_##ftype##dim(); */  \
        add_n_##ftype##dim(); \
        /* printf( "add_n: " ); print_array_##ftype##dim(); */  \
        sub_n_##ftype##dim(); \
        /* printf( "sub_n: " ); print_array_##ftype##dim(); */  \
        mul_n_##ftype##dim(); \
        /* printf( "mul_n: " ); print_array_##ftype##dim(); */  \
        div_n_##ftype##dim(); \
        /* printf( "div_n: " ); print_array_##ftype##dim(); */  \
        ftype sum_n = sum_##ftype##dim(); \
        if ( false && sum != sum_n ) \
        { \
            printf( "ERROR! n sum differs. type %s size %d, sum_n %.0lf, magnitude %.0lf, min, %.0lf max %.0lf\n", \
                    #ftype, dim, (double) sum_n, (double) magnitude, (double) min_##ftype##dim(), (double) max_##ftype##dim() ); \
            exit( 1 ); \
        } \
        ftype magnitude_n = magnitude_##ftype##dim(); \
        if ( false && magnitude != magnitude_n ) \
        { \
            printf( "ERROR! n magnitude differs. type %s size %d, sum %.0lf, magnitude_n %.0lf, min, %.0lf max %.0lf\n", \
                    #ftype, dim, (double) sum, (double) magnitude_n, (double) min_##ftype##dim(), (double) max_##ftype##dim() ); \
            exit( 1 ); \
        } \
        randomizeB_##ftype##dim(); \
        ftype aGEb = count_A_GE_B_##ftype##dim(); \
        ftype aGTb = count_A_GT_B_##ftype##dim(); \
        ftype aEQb = count_A_EQ_B_##ftype##dim(); \
        ftype aLTb = count_A_LT_B_##ftype##dim(); \
        ftype aLEb = count_A_LE_B_##ftype##dim(); \
        if ( aGEb != ( aGTb + aEQb ) ) \
        { \
            printf( "ERROR! aGEb %.0lf != GT %.0lf + EQ %.0lf\n", (double) aGEb, (double) aGTb, (double) aEQb ); \
            exit( 1 ); \
        } \
        if ( aLEb != ( aLTb + aEQb ) ) \
        { \
            printf( "ERROR! aLEb %.0lf != LT %.0lf + EQ %.0lf\n", (double) aLEb, (double) aLTb, (double) aEQb ); \
            exit( 1 ); \
        } \
        if ( dim != ( aLTb + aGEb ) ) \
        { \
            printf( "ERROR! aLTb %.0lf + GE %.0lf != dim %u\n", (double) aLTb, (double) aGEb, dim ); \
            exit( 1 ); \
        } \
        randomizeCD_##ftype##dim(); \
        add_mul_ABC_##ftype##dim(); \
        sub_mul_ABC_##ftype##dim(); \
        ftype sumC = sum_C_##ftype##dim(); \
        ftype sumD = sum_D_##ftype##dim(); \
        if ( sumC != sumD ) \
        { \
            printf( "ERROR! after mul sumC %.01f, sumD %.01f\n", (double) sumC, (double) sumD ); \
            exit( 1 ); \
        } \
        add_mul_div2_ABC_##ftype##dim(); \
        sub_mul_div2_ABC_##ftype##dim(); \
        sumC = sum_C_##ftype##dim(); \
        sumD = sum_D_##ftype##dim(); \
        if ( sumC != sumD ) \
        { \
            printf( "ERROR! after mul div2 sumC %.01f, sumD %.01f\n", (double) sumC, (double) sumD ); \
            exit( 1 ); \
        } \
        add_mul_div2b_ABC_##ftype##dim(); \
        sub_mul_div2b_ABC_##ftype##dim(); \
        sumC = sum_C_##ftype##dim(); \
        sumD = sum_D_##ftype##dim(); \
        if ( sumC != sumD ) \
        { \
            printf( "ERROR! after mul div2b sumC %.01f, sumD %.01f\n", (double) sumC, (double) sumD ); \
            exit( 1 ); \
        } \
        /*print_array_##ftype##dim();*/ \
        /*print_array_B_##ftype##dim();*/ \
        /*print_array_C_##ftype##dim();*/ \
        add_div_ABC_##ftype##dim(); \
        /*print_array_C_##ftype##dim();*/ \
        sub_div_ABC_##ftype##dim(); \
        /*print_array_C_##ftype##dim();*/ \
        sumC = sum_C_##ftype##dim(); \
        sumD = sum_D_##ftype##dim(); \
        if ( sumC != sumD ) \
        { \
            printf( "ERROR! after div sumC %.01f, sumD %.01f, type %s, dim %u\n", (double) sumC, (double) sumD, #ftype, dim ); \
            exit( 1 ); \
        } \
        inc_C_##ftype##dim(); \
        dec_C_##ftype##dim(); \
        sumC = sum_C_##ftype##dim(); \
        if ( sumC != sumD ) \
        { \
            printf( "ERROR! after sqrt/sqr sumC %.01f, sumD %.01f, type %s, dim %u\n", (double) sumC, (double) sumD, #ftype, dim ); \
            exit( 1 ); \
        } \
        return sum; \
    }

#define declare_array_operations_tests( type ) \
    array_operations_test( type, 1 ); \
    array_operations_test( type, 2 ); \
    array_operations_test( type, 3 ); \
    array_operations_test( type, 4 ); \
    array_operations_test( type, 5 ); \
    array_operations_test( type, 6 ); \
    array_operations_test( type, 7 ); \
    array_operations_test( type, 8 ); \
    array_operations_test( type, 9 ); \
    array_operations_test( type, 10 ); \
    array_operations_test( type, 11 ); \
    array_operations_test( type, 12 ); \
    array_operations_test( type, 13 ); \
    array_operations_test( type, 14 ); \
    array_operations_test( type, 15 ); \
    array_operations_test( type, 16 ); \
    array_operations_test( type, 17 ); \
    array_operations_test( type, 18 ); \
    array_operations_test( type, 19 ); \
    array_operations_test( type, 20 );

declare_array_operations_tests( int8_t );
declare_array_operations_tests( uint8_t );
declare_array_operations_tests( int16_t );
declare_array_operations_tests( uint16_t );
declare_array_operations_tests( int32_t );
declare_array_operations_tests( uint32_t );
declare_array_operations_tests( int64_t );
declare_array_operations_tests( uint64_t );

#define run_tests( type ) \
    run_##type##1(); \
    run_##type##2(); \
    run_##type##3(); \
    run_##type##4(); \
    run_##type##5(); \
    run_##type##6(); \
    run_##type##7(); \
    run_##type##8(); \
    run_##type##9(); \
    run_##type##10(); \
    run_##type##11(); \
    run_##type##12(); \
    run_##type##13(); \
    run_##type##14(); \
    run_##type##15(); \
    run_##type##16(); \
    run_##type##17(); \
    run_##type##18(); \
    run_##type##19(); \
    run_##type##20();

#define run_this_test( type ) \
    run_##type##12();

int main( int argc, char * argv[] )
{
    int loop_count = ( argc > 1 ) ? atoi( argv[ 1 ] ) : 1;
    for ( int i = 0; i < loop_count; i++ )
    {
#if 1
        run_tests( int8_t );
        run_tests( uint8_t );
        run_tests( int16_t );
        run_tests( uint16_t );
        run_tests( int32_t );
        run_tests( uint32_t );
        run_tests( int64_t );
        run_tests( uint64_t );
#else
        run_this_test( int8_t );
#endif
    }

    printf( "array operations test completed with great success\n" );
    return 0;
} //main
