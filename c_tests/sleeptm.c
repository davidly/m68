#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <chrono>
#include <cerrno>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/resource.h>

using namespace std::chrono;

//#define SLEEP_VERBOSE

// nanosleep and clock_gettime doesn't exist with this old compiler and runtime

extern "C" int nanosleep( const struct timespec * duration, struct timespec * rem );
extern "C" int clock_gettime( clockid_t id, struct timespec * res );
                             
int main( int argc, char * argv[] )
{
    high_resolution_clock::time_point tStart = high_resolution_clock::now();
    struct timespec sts_start;
    clock_gettime( CLOCK_REALTIME, &sts_start );

#ifdef SLEEP_VERBOSE
    printf( "this test should take about 2.5 seconds to complete\n" );
#endif

    uint32_t clk_tck = sysconf( _SC_CLK_TCK );
    printf( "clk_tck / number of linux ticks per second: %lu\n", clk_tck );

    printf( "sleeping for 1.5 seconds\n" );

    struct tms tstart;
    clock_t cstart = times( &tstart );
    struct timespec request = { 1, 500000000 }; // 1.5 seconds
    int result = nanosleep( &request, 0 );
    if ( -1 == result )
    {
        printf( "nanosleep failed with error %d\n", errno );
        exit( 1 );
    }

    // old C runtime implements implement high_resolution_clock on time(), which has only 1-second resolution!!!
    high_resolution_clock::time_point tAfterSleep = high_resolution_clock::now();

    struct tms tend_sleep;
    clock_t cend_sleep = times( &tend_sleep );
    clock_t cduration = cend_sleep - cstart;

#ifdef SLEEP_VERBOSE
    printf( "sleep duration %#lx = end %#lx - start %#lx\n", cduration, cend_sleep, cstart );
    printf( "  sleep duration in milliseconds: %lu\n", ( cduration * 1000 ) / clk_tck );
    printf( "  user time: %lu, kernel time: %lu\n", tend_sleep.tms_utime, tend_sleep.tms_stime );
#endif

    printf( "busy loop for 1.0 seconds\n" );
    uint32_t busy_work = 0;
    do
    {
        clock_t cbusy_loop = times( 0 );
        clock_t busy_time = ( ( cbusy_loop - cend_sleep ) * 1000 ) / clk_tck;
        if ( busy_time >= 1000 )
        {
#ifdef SLEEP_VERBOSE
            printf( "  done with busy loop; busy time: %lu\n", busy_time );
#endif
            break;
        }
        busy_work *= busy_time;
        busy_work -= 33;
        busy_work *= 14;
    } while( true );

    struct tms tend;
    clock_t cend = times( &tend );
    cduration = cend - cend_sleep;

#ifdef SLEEP_VERBOSE
    printf( "busy_work value (this will be somewhat random): %#lx\n", busy_work );
    printf( "  busy duration %#lx = end %#lx - start %#lx\n", cduration, cend, cend_sleep );
    printf( "  busy duration in milliseconds: %lu\n", ( cduration * 1000 ) / clk_tck );
#endif

    uint32_t user_ms = ( tend.tms_utime * 1000 ) / clk_tck;
    uint32_t system_ms = ( tend.tms_stime * 1000 ) / clk_tck;

#ifdef SLEEP_VERBOSE
    printf( "  user time in ms: %lu, kernel time: %lu\n", user_ms, system_ms );
#endif

    struct rusage usage;
    result = getrusage( RUSAGE_SELF, &usage );
    if ( -1 == result )
    {
        printf( "getrusage failed with error %d\n", errno );
        exit( 1 );
    }

    user_ms = ( usage.ru_utime.tv_sec * 1000 ) + ( usage.ru_utime.tv_usec / 1000 );
    system_ms = ( usage.ru_stime.tv_sec * 1000 ) + ( usage.ru_stime.tv_usec / 1000 );

    // surely some time was consumed by the busy loop
    if ( 0 == user_ms || 0 == system_ms )
        printf( "getrusage user time in ms: %lu, system time %lu\n", user_ms, system_ms );

    // old C runtime implements implement high_resolution_clock on time(), which has only 1-second resolution!!!
    high_resolution_clock::time_point tEnd = high_resolution_clock::now();
    int64_t sleepMS = duration_cast<std::chrono::milliseconds>( tAfterSleep - tStart ).count();
    int64_t totalMS = duration_cast<std::chrono::milliseconds>( tEnd - tStart ).count();

    // cut precision some slack, but this will fail with old C runtime implementation with just 1-second resolution

    if ( sleepMS < 1480 || sleepMS > 1520 || totalMS < 2480 || totalMS > 2520 )
        printf( "(chrono only accurate to 1 second) milliseconds sleeping (should be ~1500) %llu, milliseconds total (should be ~2500): %llu\n", sleepMS, totalMS );

    struct timespec sts_end;
    clock_gettime( CLOCK_REALTIME, &sts_end );

    uint32_t sts_duration = (uint32_t) ( ( ( sts_end.tv_sec - sts_start.tv_sec ) * 1000 ) + ( ( sts_end.tv_nsec - sts_start.tv_nsec ) / 1000000 ) );
    if ( sts_duration < 2480 || sts_duration > 2520 )
        printf( "millisecond duration using clock_gettime (should be about 2500 ms: %lu\n", sts_duration );

    printf( "sleepy time ended with great success\n" );
    return 0;
} //main
