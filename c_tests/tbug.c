#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
//#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <float.h>
#include <cmath>
#include <typeinfo>
#include <cstring>
#include <type_traits>


//#define _perhaps_inline __attribute__((noinline))
#define _perhaps_inline

#ifndef _WIN32
#define _countof( X ) ( sizeof( X ) / sizeof( X[0] ) )
#endif

int main( int argc, char * argv[], char * env[] )
{
    struct timeval tv;
    printf( "sizeof tv: %d, tv_sec %d, tv_us %d\n", sizeof( tv ), sizeof( tv.tv_sec ), sizeof( tv.tv_usec ) );
}

