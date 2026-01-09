#include <stdio.h>

typedef unsigned char uint8_t;
typedef char int8_t; /* DR compiler fails with a syntax error when using "signed char" */
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned long uint32_t;
typedef long int32_t;
typedef uint32_t size_t;
typedef int bool;
#define true 1
#define false 0

float f0;
float f1;
float f2;
float f4;
float f8;
float f16;
float f100;
float ftiny;
float fone16th;

char * malloc();
float fmod();
float atof();

float fpultof( x ) uint32_t x;
{
    return (float) (int32_t) x; 
}

float my_nextafterf( x, up ) float x; bool up;
{
    float next;
    int32_t i = 1;
    float finc = up ? ftiny : -ftiny;

    for ( ;; )
    {
        next = x + ( (float) i * finc );

        if ( next != x )
            return next;

        i *= 2; 
    }
    return next;
}

float my_trunc( x ) float x;
{
    return (float) ( (int32_t) x );
}

float my_fmod( x, y ) float x; float y;
{
    return x - ( my_trunc( x ) * y );
}

float fpart( x ) float x;
{
    float f = my_fmod( x, f1 );

    if ( f < f0 )
        f += f1;

    return f;
} 

float epsilon( d ) float d;
{
    return my_nextafterf( d, true ) - d;
} 

size_t powermod16( e, m ) size_t e; size_t m;
{
    size_t result, c16, b;

    if ( 1 == m )
        return 0;

    if ( 0 == e )
        return 1;

    result = 1;
    c16 = 16; 
    b = c16 % m;

    for ( ;; )
    {
        if ( e & 1 )
            result = ( result * b ) % m;

        e >>= 1;

        if ( 0 == e )
            return result;

        b = ( b * b ) % m;
    }
} 

float fun( n, j ) size_t n; size_t j;
{
    size_t denom = j;
    size_t k, p;
    float fnum, frac;
    float fdenom = j;
    float s = f0;

    for ( k = 0; k <= n; k++ )
    {
        p = powermod16( n - k, denom );
        s += ( (float) p / fdenom );
        if ( s >= f1 )
            s -= 1;
        denom += 8;
        fdenom += f8;
    }

    fnum = fone16th;

    while ( ( frac = ( fnum / fdenom ) ) > epsilon( s ) )
    {
        s += frac;
        fnum *= fone16th;
        fdenom += f8;
    }

    if ( s >= f1 )
        s -= f1;

    return s;
} 

int pi_digit( n ) size_t n;
{
    float sum, f, r;
    int x;

    sum = ( f4 * fun( n, (size_t) 1 ) ) - ( f2 * fun( n, (size_t) 4 ) ) - fun( n, (size_t) 5 ) - fun( n, (size_t) 6 );
    f = fpart( sum );
    r = f16 * f;
    x = (int) r;

    if ( x < 0 || x > 15 )
    {
        printf( "bug somewhere -- x isn't in the hex range\n" );
        exit( 1 );
    }
    return x;
} 

void Usage()
{
    printf( "Usage: pis [offset] [count]\n" );
    printf( "  PI source. Generates hexadecimal digits of PI.\n" );
    printf( "  arguments:  [offset]    Offset in 128 where generation starts. Default is 0.\n" );
    printf( "              [count]     Count in 128 of digits to generate. Default is 1.\n" );
    exit( 1 );
} 

char * AJulia1k[] =
{
    "243f6a8885a308d313198a2e03707344a4093822299f31d0082efa98ec4e6c89452821e638d01377be54",
    "66cf34e90c6cc0ac29b7c97c50dd3f84d5b5b54709179216d5d98979fb1bd1310ba698dfb5ac2ffd72db",
    "d01adfb7b8e1afed6a267e96ba7c9045f12c7f9924a19947b3916cf70801f2e2858efc16636920d87157",
    "4e69a458fea3f4933d7e0d95748f728eb658718bcd5882154aee7b54a41dc25a59b59c30d5392af26013",
    "c5d1b023286085f0ca417918b8db38ef8e79dcb0603a180e6c9e0e8bb01e8a3ed71577c1bd314b2778af",
    "2fda55605c60e65525f3aa55ab945748986263e8144055ca396a2aab10b6b4cc5c341141e8cea15486af",
    "7c72e993b3ee1411636fbc2a2ba9c55d741831f6ce5c3e169b87931eafd6ba336c24cf5c7a3253812895",
    "86773b8f48986b4bb9afc4bfe81b6628219361d809ccfb21a991487cac605dec8032ef845d5de98575b1",
    "dc262302eb651b8823893e81d396acc50f6d6ff383f442392e0b4482a484200469c8f04a9e1f9b5e21c6",
    "6842f6e96c9a670c9c61abd388f06a51a0d2d8542f68960fa728ab5133a36eef0b6c137a3be4ba3bf050",
    "7efb2a98a1f1651d39af017666ca593e82430e888cee8619456f9fb47d84a5c33b8b5ebee06f75d885c1",
    "2073401a449f56c16aa64ed3aa62363f77061bfedf72429b023d37d0d724d00a1248db0fead349f1c09b",
    "075372c980991b7b"
};

char Jul1k[1025] = { 0 };

int main( argc, argv ) int argc; char * argv[];
{
    size_t soff = 0;
    size_t soff128 = 0;
    size_t cGen128 = 1;
    size_t cGen = cGen128 * 128;
    size_t bufsize, chunkSize, startInChunks, limitInChunks, complete, generatedChunks, i, d, start;
    int x, count;
    char * ac;
    char c;

    f0 = atof( "0" );
    f1 = atof( "1" );
    f2 = atof( "2" );
    f4 = atof( "4" );
    f8 = atof( "8" );
    f16 = atof( "16" );
    f100 = atof( "100" );
    ftiny = atof( "1E-07" );
    fone16th = f1 / f16;

    if ( argc > 3 )
        Usage();

    if ( argc >= 2 )
    {
        soff128 = atol( argv[ 1 ] );
        soff = soff128 * 128;
    }

    if ( 3 == argc )
    {
        cGen128 = atol( argv[ 2 ] );
        cGen = 128 * cGen128;
    }

    printf( "soff128: %ld, soff: %ld, cGen128 %ld, cGen %ld\n",
            soff128, soff, cGen128, cGen );

    bufsize = 1 + cGen;
    ac = malloc( bufsize );
    memset( ac, 0, bufsize );

    chunkSize = 32; 
    startInChunks = ( soff128 * 128 ) / chunkSize;
    limitInChunks = ( startInChunks + ( cGen128 * 128 ) ) / chunkSize;

    printf( "startInChunks: %ld, limitInChunks %ld\n", startInChunks, limitInChunks );

    complete = 0;
    generatedChunks = cGen128 * 129 / chunkSize;

    for ( i = startInChunks; i < limitInChunks; i++ )
    {
        start = i * chunkSize;

        for ( d = start; d < start + chunkSize; d++ )
        {
            x = pi_digit( d );
            c = x <= 9 ? '0' + x : 'a' + x - 10;
            ac[ d - soff ] = c;
        }

        complete++;
        printf( "percent complete: %d\n", (int) ( f100 * (float) complete / (float) generatedChunks ) );
    }

    if ( 0 == soff && cGen128 >= 1 )
    {
        Jul1k[ 0 ] = 0;
        count = sizeof( AJulia1k ) / sizeof( AJulia1k[ 0 ] );
        for ( i = 0; i < count; i++ )
            strcat( Jul1k, AJulia1k[ i ] );

        count = cGen128 * 128;
        if ( count > 1024 )
            count = 1024;

        if ( strncmp( ac, Jul1k, count ) )
            printf( "results for length %u don't match Julia!\n", count );
        else
            printf( "results are valid for length %u\n", count );
    }

    printf( "final: %s\n", ac );
    return 0;
} 

