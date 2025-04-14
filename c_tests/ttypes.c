#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <float.h>
#include <cmath>
#include <typeinfo>
#include <cstring>
#include <type_traits>

typedef long double ldouble_t;

//#define _perhaps_inline __attribute__((noinline))
#define _perhaps_inline

#define _countof( X ) ( sizeof( X ) / sizeof( X[0] ) )

bool IS_FP( float x ) { return true; }    
bool IS_FP( double x ) { return true; }    
bool IS_FP( ldouble_t x ) { return true; }    
bool IS_FP( int8_t x ) { return false; }
bool IS_FP( uint8_t x ) { return false; }
bool IS_FP( int16_t x ) { return false; }
bool IS_FP( uint16_t x ) { return false; }
bool IS_FP( int32_t x ) { return false; }
bool IS_FP( uint32_t x ) { return false; }
bool IS_FP( int64_t x ) { return false; }
bool IS_FP( uint64_t x ) { return false; }

template <class T> T do_abs( T x )
{
    return ( x < 0 ) ? -x : x;
}

const char * maptype( const char * p )
{
    switch( *p )
    {
        case 'a' : return "int8";
        case 'h' : return "uint8";
        case 's' : return "int16";
        case 't' : return "uint16";
        case 'x' : return "int64";
        case 'y' : return "uint64";
        case 'i' : return "int32";
        case 'j' : return "uint32";
        case 'l' : return "int32";
        case 'm' : return "uint32";
        case 'n' : return "int128";
        case 'o' : return "uint128";
        case 'f' : return "float";
        case 'd' : return "double";
        case 'e' : return "ldouble";
    }
    return "unknown";
} //maptype

template <class T, class U> T do_cast( U x )
{
    size_t cbU = sizeof( U );
    size_t cbT = sizeof( T );
    bool signedU = std::is_signed<T>();
    bool signedT = std::is_signed<U>();
    T result = 0;

    if ( IS_FP( result ) )
    {
        if ( 4 == cbT )
            result = (T) ( ( x < FLT_MIN ) ? FLT_MIN : ( x > FLT_MAX ) ? FLT_MAX : x );
        else if ( 8 == cbT )
            result = (T) ( ( x < DBL_MIN ) ? DBL_MIN : ( x > DBL_MAX ) ? DBL_MAX : x );
        else if ( 16 == cbT || 12 == cbT || 10 == cbT )
            result = (T) ( ( x < LDBL_MIN ) ? LDBL_MIN : ( x > LDBL_MAX ) ? LDBL_MAX : x );
        else
            printf( "unknown floating point type\n" );
    }
    else
    {
        if ( 1 == cbT )
        {
            if ( signedT )
                result = (T) ( ( x < INT8_MIN ) ? INT8_MIN : ( x > INT8_MAX ) ? INT8_MAX : x );
            else
                result = (T) ( ( x < 0 ) ? 0 : ( x > UINT8_MAX ) ? UINT8_MAX : x );
        }
        else if ( 2 == cbT )
        {
            if ( signedT )
                result = (T) ( ( x < INT16_MIN ) ? INT16_MIN : ( x > INT16_MAX ) ? INT16_MAX : x );
            else
                result = (T) ( ( x < 0 ) ? 0 : ( x > UINT16_MAX ) ? UINT16_MAX : x );
        }
        else if ( 4 == cbT )
        {
            if ( signedT )
                result = (T) ( ( x < INT32_MIN ) ? INT32_MIN : ( x > INT32_MAX ) ? INT32_MAX : x );
            else
                result = (T) ( ( x < 0 ) ? 0 : ( x > UINT32_MAX ) ? UINT32_MAX : x );
        }
        else if ( 8 == cbT )
        {
            if ( signedT )
                result = (T) ( ( x < INT64_MIN ) ? INT64_MIN : ( x > INT64_MAX ) ? INT64_MAX : x );
            else
                result = (T) ( ( x < 0 ) ? 0 : ( x > UINT64_MAX ) ? UINT64_MAX : x );
        }
        else
            printf( "unknown integer type\n" );
    }
    return result;
}

template <class T> _perhaps_inline T do_sum( T array[], size_t size )
{
    T sum = 0;
    for ( int i = 0; i < size; i++ )
    {
        //printf( "in do_sum, element %d %.12g\n", i, (double) array[ i ] );
        sum += array[ i ];
    }
    /*printf( "sum in do_sum: %.12g\n", (double) sum );*/
    return sum;
}

template <class T> void printBytes( const char * msg, T * p, size_t size )
{
    printf( "%s\n", msg );
    uint8_t * pb = (uint8_t *) p;
    size_t cb = sizeof( T );
    for ( size_t i = 0; i < size; i++ )
    {
        printf( "    element %zd: ", i );
        size_t j = cb;
        do
        {
            j--;
            printf( "%02x", pb[ j + ( i * cb ) ] );
        } while ( j != 0 );
        printf( "\n" );
    }
}

template <class T, class U, size_t size> T tstCasts( T t, U u )
{
    T a[ size ] = { 0 };
    U b[ _countof( a ) ] = { 0 };
    T c[ _countof( a ) ] = { 0 };
    T x = t;

    srand( 0 );

    for ( int i = 0; i < _countof( a ); i++ )
    {
        x = x + do_cast<T,size_t>( ( rand() % ( i + 1000 ) ) / 2 );
        x = -x;
        x = do_cast<T,int64_t>( (int64_t) x & 0x33303330333033 );
        x = do_abs( x );
        x = do_cast<T,double>( sqrt( (double) x ) );
        x += do_cast<T,float>( 1.02f );
        x = do_cast<T,double>( (double) x * 3.2f );
        u += do_cast<U,size_t>( ( rand() % ( i + 2000 ) ) / 3 );
        a[ i ] = ( x * do_cast<T,U>( u ) ) + ( x + do_cast<T,U>( u ) );
        //printf( "bottom of loop, a[%d] is %.12g, u %.12g, x %.12g\n", i, (double) a[ i ], (double) u, (double) x );
        //printBytes( "array a:", a, size );
    }

    //syscall( 0x2002, 1 );        
    for ( int i = 0; i < _countof( a ); i++ )
    {
        //if ( 16 == sizeof( U ) && 16 == sizeof( T ) )
        //    syscall( 0x2002, 1 );        

        T absolute = do_abs( a[ i ] );
        b[ i ] = do_cast<U,T>( absolute * (T) 2.2 );
        c[ i ] = absolute * (T) 4.4;
        //printf( "b[%d] = %.12g, a = %.12g, absolute = %.12g\n", i, (double) b[i], (double) a[i], (double) absolute );
        //printf( "c[%d] = %.12g, a = %.12g, absolute = %.12g\n", i, (double) c[i], (double) a[i], (double) absolute );
    }
    //syscall( 0x2002, 0 );        
    
    T sumA = do_sum( a, _countof( a ) );
    //syscall( 0x2002, 1 );        
    U sumB = do_sum( b, _countof( b ) );
    T sumC = do_sum( c, _countof( c ) );
    //printf( "sumC: %f = %#x\n", sumC, * (uint32_t *) &sumC );
    
    x = sumA / 128;
    
    // use 6 digits of precision for float and 12 for everything else

    int t_precision = std::is_same<T,float>::value ? 6 : 12;
    int u_precision = std::is_same<U,float>::value ? 6 : 12;
    printf( "cast:     types %7s + %7s, size %d, sumA %.*lf, sumB %.*lf, sumC %.*lf\n", 
            maptype( typeid(T).name() ), maptype( typeid(U).name() ), 
            size, t_precision, (double) sumA, u_precision, (double) sumB, t_precision, (double) sumC );
    
    //syscall( 0x2002, 0 );        
#if 0
    for ( int i = 0; i < _countof( a ); i++ )
    {
        printf( "a[%d] = %.12g %d\n", i, (double) a[i], (int) a[i] );
        printf( "b[%d] = %.12g %d\n", i, (double) b[i], (int) b[i] );
        //printf( "c[%d] = %.12g %d\n", i, (double) c[i], (int) c[i] );
    }
#endif     
        
    return x;
} //tstCasts

template <class T, class U, size_t size> T tstOverflows( T t, U u )
{
    T a[ size ] = { 0 };
    U b[ _countof( a ) ] = { 0 };
    T c[ _countof( a ) ] = { 0 };
    T x = t;

    srand( 0 );

    for ( int i = 0; i < _countof( a ); i++ )
    {
        x += ( rand() % ( i + 1000 ) ) / 2;
        x = -x;
        x = (int64_t) x & 0x33303330333033;
        x = do_abs( x );
        x = (T) sqrt( (double) x );
        x += (T) 1.02;
        x = (T) ( (double) x * 3.2 );
        u += ( rand() % ( i + 2000 ) ) / 3;
        a[ i ] = ( x * (T) u ) + ( x + (T) u );
        //printf( "bottom of loop, a[%d] is %.12g, u %.12g, x %.12g\n", i, (double) a[ i ], (double) u, (double) x );
    }

    //syscall( 0x2002, 1 );        
    for ( int i = 0; i < _countof( a ); i++ )
    {
        T absolute = do_abs( a[ i ] );
        // overflow with b[] results in many differences across compilers and platforms
        // so always do_cast to avoid implementation-specific differences in C++ compilers
        //b[ i ] = (U)( absolute * (T) 2.2 );
        b[ i ] = do_cast<U,T>( absolute * (T) 2.2 );
        c[ i ] = absolute * (T) 4.4;
        //printf( "b[%d] = %.12g, a = %.12g\n", i, (double) b[i], (double) a[i] );
    }
    //syscall( 0x2002, 0 );
    
    T sumA = do_sum( a, _countof( a ) );
    //syscall( 0x2002, 1 );        
    U sumB = do_sum( b, _countof( b ) );
    T sumC = do_sum( c, _countof( c ) );
    
    x = sumA / 128;
    
    // use 6 digits of precision for float and 12 for everything else

    int t_precision = std::is_same<T,float>::value ? 6 : 12;
    int u_precision = std::is_same<U,float>::value ? 6 : 12;
    printf( "overflow: types %7s + %7s, size %d, sumA %.*lf, sumB %.*lf, sumC %.*lf\n", 
            maptype( typeid(T).name() ), maptype( typeid(U).name() ), 
            size, t_precision, (double) sumA, u_precision, (double) sumB, t_precision, (double) sumC );
    
    //syscall( 0x2002, 0 );        
#if 0
    for ( int i = 0; i < _countof( a ); i++ )
    {
        //printf( "a[%d] = %.12g %d\n", i, (double) a[i], (int) a[i] );
        printf( "b[%d] = %.12g %d\n", i, (double) b[i], (int) b[i] );
        //printf( "c[%d] = %.12g %d\n", i, (double) c[i], (int) c[i] );
    }
#endif     
        
    return x;
} //tstOverflows

#if 0

template <class T, class U, size_t size> T tstAssignments( T t, U u )
{
    T a[ size ] = { 0 };
    U b[ _countof( a ) ] = { 0 };
    U c[ _countof( a ) ] = { 0 };
    srand( 0 );

    memset( &a, 0x22, sizeof( a ) );
    memset( &b, 0x66, sizeof( b ) );

    for ( int i = 0; i < _countof( a ); i++ )
    {
        a[ i ] += do_cast<T,size_t>( ( rand() % 111 ) - 55 );
        b[ i ] += do_cast<U,T>( a[ i ] );
        c[ i ] = (U) a[ i ];
    }

    T sumA = do_sum( a, _countof( a ) );
    U sumB = do_sum( b, _countof( b ) );
    U sumC = do_sum( c, _countof( c ) );
    
    T x = sumA / 128;
    
    // use 6 digits of precision for float and 12 for everything else

    int t_precision = std::is_same<T,float>::value ? 6 : 12;
    int u_precision = std::is_same<U,float>::value ? 6 : 12;
    printf( "assignment: types %7s + %7s, size %d, sumA %.*g, sumB %.*g, sumC %.*g\n", 
        maptype( typeid(T).name() ), maptype( typeid(U).name() ), 
        size, t_precision, (double) sumA, u_precision, (double) sumB, u_precision, (double) sumC );


    return x;            
} //tstAssignments

#endif

template <class T, class U, size_t size> T tst( T t, U u )
{
    T result = 0;
    result += tstCasts<T,U,size>( t, u );
    result += tstOverflows<T,U,size>( t, u );
    //result += tstAssignments<T,U,size>( t, u );
    return result;
}

#define run_tests( ftype, dim ) \
  tst<ftype,int8_t,dim>( 0, 0 ); \
  tst<ftype,uint8_t,dim>( 0, 0 ); \
  tst<ftype,int16_t,dim>( 0, 0 ); \
  tst<ftype,uint16_t,dim>( 0, 0 ); \
  tst<ftype,int32_t,dim>( 0, 0 ); \
  tst<ftype,uint32_t,dim>( 0, 0 ); \
  tst<ftype,int64_t,dim>( 0, 0 ); \
  tst<ftype,uint64_t,dim>( 0, 0 ); \
  tst<ftype,float,dim>( 0, 0 ); \
  tst<ftype,double,dim>( 0, 0 ); \
  tst<ftype,ldouble_t,dim>( 0, 0 ); 

#define run_testsz( ftype, dim ) \
  tst<ftype,int16_t,dim>( 0, 0 ); \
  tst<ftype,uint16_t,dim>( 0, 0 ); \
  tst<ftype,int32_t,dim>( 0, 0 ); \
  tst<ftype,uint32_t,dim>( 0, 0 ); \
  tst<ftype,int64_t,dim>( 0, 0 ); \
  tst<ftype,uint64_t,dim>( 0, 0 );

#define run_tests_one( ftype, dim ) \
  tst<ftype,float,dim>( 0, 0 );

#define run_dimension( dim ) \
  run_tests( int8_t, dim ); \
  run_tests( uint8_t, dim ); \
  run_tests( int16_t, dim ); \
  run_tests( uint16_t, dim ); \
  run_tests( int32_t, dim ); \
  run_tests( uint32_t, dim ); \
  run_tests( int64_t, dim ); \
  run_tests( uint64_t, dim ); \
  run_tests( float, dim ); \
  run_tests( double, dim ); \
  run_tests( ldouble_t, dim );

  #define run_dimensionz( dim ) \
  run_testsz( uint64_t, dim );

#define run_dimension_one( dim ) \
  run_tests_one( int64_t, dim );

int main( int argc, char * argv[], char * env[] )
{
#if 0    
    printf( "types: i8 %s, ui8 %s, i16 %s, ui16 %s, i32 %s, ui32 %s, i64 %s, ui64 %s, f %s, d %s, ld %s\n",
            typeid(int8_t).name(), typeid(uint8_t).name(), typeid(int16_t).name(), typeid(uint16_t).name(),
            typeid(int32_t).name(), typeid(uint32_t).name(), typeid(int64_t).name(), typeid(uint64_t).name(),
            typeid(float).name(), typeid(double).name(), typeid(ldouble_t).name() );
#endif            

#if 1
    run_dimension( 2 );    
    run_dimension( 3 );    
    run_dimension( 4 );    
    run_dimension( 5 );    
    run_dimension( 6 );    
    run_dimension( 15 );    
    run_dimension( 16 );    
    run_dimension( 17 );    
    run_dimension( 31 );    
    run_dimension( 32 );    
    run_dimension( 33 );    
    run_dimension( 128 );
#else    
    //run_dimensionz( 3 );    
    tst<float,uint32_t,2>( 0, 0 );
    //tst<int128_t,int128_t,3>( 0, 0 );
#endif

    printf( "test types completed with great success\n" );
    return 0;
}
