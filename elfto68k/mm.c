/* BYTE magazine October 1982. Jerry Pournelle. */
/* ported to C and added test cases by David Lee */
/* various bugs not found because dimensions are square fixed by David Lee */
/* expected result for 20/20/20: 4.65880E+05 */
/* 20/20/20 float version runs in 13 seconds on the original PC */

/* why so many matrix size and datatype variants? g++ produces different code to optimize for each case */

#define LINT_ARGS

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <cmath>

#define _perhaps_inline __attribute__((noinline))
//#define _perhaps_inline

typedef long double ldouble_t;

template <class T> T do_abs( T x )
{
    return ( x < 0 ) ? -x : x;
}

#define matrix_test( ftype, dim ) \
    ftype A_##ftype##dim[ dim ][ dim ]; \
    ftype B_##ftype##dim[ dim ][ dim ]; \
    ftype C_##ftype##dim[ dim ][ dim ]; \
    _perhaps_inline void fillA_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            for ( int j = 0; j < dim; j++ ) \
                A_##ftype##dim[ i ][ j ] = (ftype) ( i + j + 2 ); \
    } \
    _perhaps_inline void fillB_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            for ( int j = 0; j < dim; j++ ) \
                B_##ftype##dim[ i ][ j ] = (ftype) ( ( i + j + 2 ) / ( j + 1 ) ); \
    } \
    _perhaps_inline void fillC_##ftype##dim() \
    { \
        for ( int i = 0; i < dim; i++ ) \
            for ( int j = 0; j < dim; j++ ) \
                C_##ftype##dim[ i ][ j ] = (ftype) 0; \
    } \
    _perhaps_inline void print_array_##ftype##dim( ftype a[dim][dim] ) \
    { \
        printf( "array: \n" ); \
        for ( int i = 0; i < dim; i++ ) \
        { \
            for ( int j = 0; j < dim; j++ ) \
                printf( " %Lf", (long double) a[ i ][ j ] ); \
            printf( "\n" ); \
        } \
    } \
    _perhaps_inline void matmult_##ftype##dim() \
    { \
        /* this debugging line causes the compiler to optimize code differently (tbl/zip)! syscall( 0x2002, 1 ); */ \
        for ( int i = 0; i < dim; i++ ) \
            for ( int j = 0; j < dim; j++ ) \
                for ( int k = 0; k < dim; k++ ) \
                    C_##ftype##dim[ i ][ j ] += A_##ftype##dim[ i ][ k ] * B_##ftype##dim[ k ][ j ]; \
    } \
    _perhaps_inline ftype fmod_nonsense_##ftype##dim() /* find fmods */ \
    { \
        ftype FM_##ftype##dim[ dim ][ dim ]; \
        for ( int i = 0; i < dim; i++ ) \
            for ( int j = 0; j < dim; j++ ) \
                FM_##ftype##dim[ i ][ j ] = fmod( C_##ftype##dim[ i ][ j ], 3.2 ); \
        ftype m = -1; \
        ftype sum = 0; \
        for ( int i = 0; i < dim; i++ ) \
            for ( int j = 0; j < dim; j++ ) \
            { \
                sum += FM_##ftype##dim[ i ][ j ]; \
                if ( FM_##ftype##dim[ i ][ j ] > m ) \
                    m = FM_##ftype##dim[ i ][ j ]; \
            } \
        /*print_array_##ftype##dim( FM_##ftype##dim );*/ \
        return sum; \
    } \
    _perhaps_inline ftype dotsum_nonsense_##ftype##dim() /* find fmods */ \
    { \
        ftype dotsum = 0; \
        ftype negsum = 0; \
        for ( int i = 0; i < dim; i++ ) \
            for ( int j = 0; j < dim; j++ ) \
            { \
                dotsum += ( A_##ftype##dim[ i ][ j ] * B_##ftype##dim[ i ][ j ] ); \
                negsum += ( -A_##ftype##dim[ i ][ j ] * B_##ftype##dim[ i ][ j ] ); \
            } \
        return dotsum; \
    } \
    _perhaps_inline void div_nonsense_##ftype##dim() /* force more instructions to be generated with this nonsense */ \
    { \
        for ( int i = 0; i < dim; i++ ) \
        { \
            for ( int j = 0; j < dim; j++ ) \
                for ( int k = 0; k < dim; k++ ) \
                    if ( B_##ftype##dim[ k ][ j ] != 0 ) \
                    { \
                        C_##ftype##dim[ i ][ j ] += A_##ftype##dim[ i ][ k ] / B_##ftype##dim[ k ][ j ]; \
                        C_##ftype##dim[ i ][ j ] -= A_##ftype##dim[ i ][ k ] * B_##ftype##dim[ k ][ j ]; \
                        C_##ftype##dim[ i ][ j ] -= A_##ftype##dim[ i ][ k ] / B_##ftype##dim[ k ][ j ]; \
                        C_##ftype##dim[ i ][ j ] += A_##ftype##dim[ i ][ k ] * B_##ftype##dim[ k ][ j ]; \
                    } \
        } \
    } \
    _perhaps_inline ftype math_nonsense_##ftype##dim() /* force more instructions to be generated with this nonsense */ \
    { \
        ftype D[ dim ][ dim ]; \
        ftype E[ dim ][ dim ]; \
        for ( int i = 0; i < dim; i++ ) \
            for ( int j = 0; j < dim; j++ ) \
            { \
                D[ i ][ j ] = C_##ftype##dim[ i ][ j ]; \
                E[ i ][ j ] = (ftype) ( i + j * 10 ); \
            } \
        for ( int i = 0; i < dim; i++ ) \
            for ( int j = 0; j < dim; j++ ) \
                E[ i ][ j ] *= (ftype) 1.6; \
        for ( int i = 0; i < dim; i++ ) \
            for ( int j = 0; j < dim; j++ ) \
                D[ i ][ j ] *= (ftype) ( ( (ftype) 1.6 ) + ( E[ i ][ j ] / (ftype) 1.1 ) ); \
        for ( int i = 0; i < dim; i++ ) \
            for ( int j = 0; j < dim; j++ ) \
                D[ i ][ j ] /= (ftype) 1.07; \
        ftype result = (ftype) 0; \
        for ( int i = 0; i < dim; i++ ) \
            for ( int j = 0; j < dim; j++ ) \
                result += (ftype) ( D[ i ][ j ] ); \
        return result; /* resuls vary on Arm hardware for different compilers and optimization levels due to rounding. */ \
    } \
    _perhaps_inline ftype sum_##ftype##dim() \
    { \
        ftype result = (ftype) 0; \
        for ( int i = 0; i < dim; i++ ) \
            for ( int j = 0; j < dim; j++ ) \
                result += C_##ftype##dim[ i ][ j ]; \
        return result; \
    } \
    _perhaps_inline ftype magnitude_##ftype##dim() \
    { \
        ftype result = (ftype) 0; \
        for ( int i = 0; i < dim; i++ ) \
            for ( int j = 0; j < dim; j++ ) \
                result += do_abs( C_##ftype##dim[ i ][ j ] ); \
        return result; \
    } \
    _perhaps_inline ftype min_##ftype##dim() \
    { \
        ftype result = C_##ftype##dim[ 0 ][ 0 ]; \
        for ( int i = 0; i < dim; i++ ) \
            for ( int j = 0; j < dim; j++ ) \
                result = C_##ftype##dim[ i ][ j ] < result ? C_##ftype##dim[ i ][ j ] : result; \
        return result; \
    } \
    _perhaps_inline ftype max_##ftype##dim() \
    { \
        ftype result = C_##ftype##dim[ 0 ][ 0 ]; \
        for ( int i = 0; i < dim; i++ ) \
            for ( int j = 0; j < dim; j++ ) \
                result = C_##ftype##dim[ i ][ j ] > result ? C_##ftype##dim[ i ][ j ] : result; \
        return result; \
    } \
    _perhaps_inline ftype sqrt_sum_##ftype##dim() \
    { \
        ftype result = 0; \
        for ( int i = 0; i < dim; i++ ) \
            for ( int j = 0; j < dim; j++ ) \
                result += (ftype) sqrt( (double) do_abs( C_##ftype##dim[ i ][ j ] ) ); \
        return result; \
    } \
    ftype run_##ftype##dim() \
    { \
        fillA_##ftype##dim(); \
        fillB_##ftype##dim(); \
        fillC_##ftype##dim(); \
        matmult_##ftype##dim(); \
        /*print_array_##ftype##dim( A_##ftype##dim );*/ \
        /*print_array_##ftype##dim( B_##ftype##dim );*/ \
        /*print_array_##ftype##dim( C_##ftype##dim );*/ \
        ftype sum = sum_##ftype##dim(); \
        ftype magnitude = magnitude_##ftype##dim(); \
        div_nonsense_##ftype##dim(); \
        ftype mathnonsense = math_nonsense_##ftype##dim(); \
        ftype fmodsum = fmod_nonsense_##ftype##dim(); \
        ftype dotsum = dotsum_nonsense_##ftype##dim(); \
        ftype nonsense_sum = sum_##ftype##dim(); \
        ftype sqrtsum = sqrt_sum_##ftype##dim(); \
        printf( "%s dim %d: sum %.1lf, mag %.1lf, min, %.1lf max %.1lf, fmodsum %.3lf, dotsum %.1lf, sqrtsum %.1lf\n", \
                #ftype, dim, (double) sum, (double) magnitude, (double) min_##ftype##dim(), (double) max_##ftype##dim(), \
                (double) fmodsum, (double) dotsum, (double) sqrtsum ); \
        if ( sum != nonsense_sum ) \
        { \
            printf( "nonsense differs: %lf\n", (double) nonsense_sum ); \
            exit( 1 ); \
        } \
        return sum; \
    }

#define declare_matrix_tests( type ) \
    matrix_test( type, 1 ); \
    matrix_test( type, 2 ); \
    matrix_test( type, 3 ); \
    matrix_test( type, 4 ); \
    matrix_test( type, 5 ); \
    matrix_test( type, 6 ); \
    matrix_test( type, 7 ); \
    matrix_test( type, 8 ); \
    matrix_test( type, 9 ); \
    matrix_test( type, 10 ); \
    matrix_test( type, 11 ); \
    matrix_test( type, 12 ); \
    matrix_test( type, 13 ); \
    matrix_test( type, 14 ); \
    matrix_test( type, 15 ); \
    matrix_test( type, 16 ); \
    matrix_test( type, 17 ); \
    matrix_test( type, 18 ); \
    matrix_test( type, 19 ); \
    matrix_test( type, 20 );

declare_matrix_tests( float );
declare_matrix_tests( double );
declare_matrix_tests( ldouble_t );
declare_matrix_tests( int8_t );
declare_matrix_tests( uint8_t );
declare_matrix_tests( int16_t );
declare_matrix_tests( uint16_t );
declare_matrix_tests( int32_t );
declare_matrix_tests( uint32_t );
declare_matrix_tests( int64_t );
declare_matrix_tests( uint64_t );

#define run_tests( type, format ) \
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

#define loud_run_tests( type, format ) \
    printf( "matrix %s 1: " format "\n", #type, run_##type##1() ); \
    printf( "matrix %s 2: " format "\n", #type, run_##type##2() ); \
    printf( "matrix %s 3: " format "\n", #type, run_##type##3() ); \
    printf( "matrix %s 4: " format "\n", #type, run_##type##4() ); \
    printf( "matrix %s 5: " format "\n", #type, run_##type##5() ); \
    printf( "matrix %s 6: " format "\n", #type, run_##type##6() ); \
    printf( "matrix %s 7: " format "\n", #type, run_##type##7() ); \
    printf( "matrix %s 8: " format "\n", #type, run_##type##8() ); \
    printf( "matrix %s 9: " format "\n", #type, run_##type##9() ); \
    printf( "matrix %s 10: " format "\n", #type, run_##type##10() ); \
    printf( "matrix %s 11: " format "\n", #type, run_##type##11() ); \
    printf( "matrix %s 12: " format "\n", #type, run_##type##12() ); \
    printf( "matrix %s 13: " format "\n", #type, run_##type##13() ); \
    printf( "matrix %s 14: " format "\n", #type, run_##type##14() ); \
    printf( "matrix %s 15: " format "\n", #type, run_##type##15() ); \
    printf( "matrix %s 16: " format "\n", #type, run_##type##16() ); \
    printf( "matrix %s 17: " format "\n", #type, run_##type##17() ); \
    printf( "matrix %s 18: " format "\n", #type, run_##type##18() ); \
    printf( "matrix %s 19: " format "\n", #type, run_##type##19() ); \
    printf( "matrix %s 20: " format "\n", #type, run_##type##20() );

#define run_this_test( type ) \
    run_##type##4();

int main( int argc, char * argv[] )
{
    int loop_count = ( argc > 1 ) ? atoi( argv[ 1 ] ) : 1;
    for ( int i = 0; i < loop_count; i++ )
    {
#if 1
        run_tests( float, "%f");
        run_tests( double, "%lf");
        run_tests( ldouble_t, "%Lf");
        run_tests( int8_t, "%d");
        run_tests( uint8_t, "%u");
        run_tests( int16_t, "%d");
        run_tests( uint16_t, "%u");
        run_tests( int32_t, "%d");
        run_tests( uint32_t, "%u");
        run_tests( int64_t, "%lld");
        run_tests( uint64_t, "%llu");
#else    
        run_this_test( uint8_t );
#endif    
    }


    printf( "matrix multiply test completed with great success\n" );
    return 0;
} //main
