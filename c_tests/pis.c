#include <stdio.h>
#include <cmath>
#include <cstring>
#include <cfloat>
#include <assert.h>
#include <new>

using namespace std;

double fpart_nomod( double x )
{
    //printf( "finding fpart_nomod of %lf\n", x );
    assert( x < 2.0 );
    assert( x >= 0.0 );

    double d;

    if ( x >= 1.0 )
        d = x - 1.0;
    else
        d = x;

    //printf( "fpart of x %.*lf is %.*lf\n", 20, x, 20, d );
    return d;
} //fpart_nomod

double fpart( double x )
{
    double d = fmod( x, 1.0 );

    if ( d < 0.0 )
        d = 1.0 + d;

    //printf( "fpart of x %.*lf is %.*lf\n", 20, x, 20, d );
    return d;
} //fpart

double eps( double d )
{
    double n = nextafter( d, DBL_MAX );
    double r = n - d;

    //printf( "  n %.*lf eps of %.*lf is %.*lf\n", 20, n, 20, d, 20, r );
    return r;
} //eps

size_t powermod16_faster( size_t e, size_t m )
{
    // https://en.wikipedia.org/wiki/Modular_exponentiation
    // faster way to calculate b^e % m

    if ( 1 == m )
        return 0;

    if ( 0 == e )
        return 1;

    size_t result = 1;
    size_t b = 16 % m;

    do
    {
        if ( e & 1 )
            result = ( result * b ) % m;

        e >>= 1;

        if ( 0 == e )
            return result;

        b = ( b * b ) % m;
    } while ( true );
} //powermod16_faster

double fun( size_t n, size_t j )
{
    //printf( "fun call n %zd, j %zd\n", n, j );
    double s = 0.0;
    size_t denom = j;

    for ( size_t k = 0; k <= n; k++ )
    {
        size_t p = powermod16_faster( n - k, denom );
        //printf( "in kloop, s %lf p %zd, denom %zd\n", s, p, denom );
        s = fpart_nomod( s + ( (double) p / (double) denom ) );
        //printf( "  in kloop:  k %zd, p %zd, denom %lf incremental s %lf\n", k, p, denom, s );
        denom += 8;
    }

    //printf( "after kloop: n %zd, j %zd, s %lf\n", n, j, s );

    double num = 1.0 / 16.0;
    double fdenom = (double) denom;
    double frac;

    while ( ( frac = ( num / fdenom ) ) > eps( s ) )
    {
        //printf( "  frac: %.*lf, would-be result %.*lf\n", 20, frac, 20, fpart_nomod( s ) );
        s += frac;
        num /= 16.0;
        fdenom += 8.0;
    }

    //printf( "final frac %.*lf and s prior to fpart %.*lf fpart_nomod(s) %.*lf\n", 20, frac, 20, s, 20, fpart( s ) );
    return fpart_nomod( s );
} //fun

int pi_digit( size_t n )
{
    double sum = ( 4.0 * fun( n, 1 ) ) - ( 2.0 * fun( n, 4 ) ) - fun( n, 5 ) - fun( n, 6 );
    double f = fpart( sum );
    double r = 16.0 * f;
    int x = (int) r;

    //printf( "resulting sum %lf, f %lf, r %lf, x: %d\n", sum, f, r, x );
    assert( x >= 0 && x <= 15 );
    return x;
} //pi_digit

void Usage()
{
    printf( "Usage: pis [offset] [count]\n" );
    printf( "  PI source. Generates hexadecimal digits of PI.\n" );
    printf( "  arguments:  [offset]    Offset in 128 where generation starts. Default is 0.\n" );
    printf( "              [count]     Count in 128 of digits to generate. Default is 1.\n" );
    exit( 1 );
} //Usage

int main( int argc, char * argv[] )
{
    // These are in units of 128

    size_t startingOffset = 0;
    size_t startingOffset128 = 0;
    size_t countGenerated128 = 1;
    size_t countGenerated = countGenerated128 * 128;

    if ( argc > 3 )
        Usage();

    if ( argc >= 2 )
    {
        startingOffset128 = atoll( argv[ 1 ] );
        startingOffset = startingOffset128 * 128;
    }

    if ( 3 == argc )
    {
        countGenerated128 = atoll( argv[ 2 ] );
        countGenerated = 128 * countGenerated128;
    }

    printf( "startingOffset128: %d, startingOffset: %d, countGenerated128 %d, countGenerated %d\n",
            startingOffset128, startingOffset, countGenerated128, countGenerated );

    size_t bufsize = 1 + countGenerated;
    char* ac = new char[ bufsize ];
    memset( ac, 0, bufsize );

    const size_t chunkSize = 32; // rely on fact that 32*3 = 128
    size_t startInChunks = ( startingOffset128 * 128 ) / chunkSize;
    size_t limitInChunks = ( startInChunks + ( countGenerated128 * 128 ) ) / chunkSize;

    printf( "startInChunks: %d, limitInChunks %d\n", startInChunks, limitInChunks );

    size_t complete = 0;
    size_t generatedChunks = countGenerated128 * 129 / chunkSize;

    for ( size_t i = startInChunks; i < limitInChunks; i++ )
    {
        size_t start = i * chunkSize;

        for ( size_t d = start; d < start + chunkSize; d++ )
        {
            int x = pi_digit( d );
            char c = x <= 9 ? '0' + x : 'a' + x - 10;
            ac[ d - startingOffset ] = c;
        }

        complete++;
        printf( "percent complete: %lf\n", 100.0 * (double) complete / (double) generatedChunks );
    };

    if ( 0 == startingOffset && countGenerated128 >= 1 )
    {
        const char* Julia1k =
            "243f6a8885a308d313198a2e03707344a4093822299f31d0082efa98ec4e6c89452821e638d01377be54"
            "66cf34e90c6cc0ac29b7c97c50dd3f84d5b5b54709179216d5d98979fb1bd1310ba698dfb5ac2ffd72db"
            "d01adfb7b8e1afed6a267e96ba7c9045f12c7f9924a19947b3916cf70801f2e2858efc16636920d87157"
            "4e69a458fea3f4933d7e0d95748f728eb658718bcd5882154aee7b54a41dc25a59b59c30d5392af26013"
            "c5d1b023286085f0ca417918b8db38ef8e79dcb0603a180e6c9e0e8bb01e8a3ed71577c1bd314b2778af"
            "2fda55605c60e65525f3aa55ab945748986263e8144055ca396a2aab10b6b4cc5c341141e8cea15486af"
            "7c72e993b3ee1411636fbc2a2ba9c55d741831f6ce5c3e169b87931eafd6ba336c24cf5c7a3253812895"
            "86773b8f48986b4bb9afc4bfe81b6628219361d809ccfb21a991487cac605dec8032ef845d5de98575b1"
            "dc262302eb651b8823893e81d396acc50f6d6ff383f442392e0b4482a484200469c8f04a9e1f9b5e21c6"
            "6842f6e96c9a670c9c61abd388f06a51a0d2d8542f68960fa728ab5133a36eef0b6c137a3be4ba3bf050"
            "7efb2a98a1f1651d39af017666ca593e82430e888cee8619456f9fb47d84a5c33b8b5ebee06f75d885c1"
            "2073401a449f56c16aa64ed3aa62363f77061bfedf72429b023d37d0d724d00a1248db0fead349f1c09b"
            "075372c980991b7b";

        if ( countGenerated128 >= 8 && strncmp( ac, Julia1k, 1024 ) )
            printf( "results 1k don't match Julia!\n" );
        else if ( strncmp( ac, Julia1k, 128 ) )
            printf( "results 128 don't match Julia!\n" );
        else
            printf( "results are valid\n" );
    }

    printf( "final: %s\n", ac );
    return 0;
} //main

