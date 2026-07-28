#include <stdio.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

#define BDOS_CONIN       1
#define BDOS_CONOUT      2
#define BDOS_READER      3
#define BDOS_PUNCH       4
#define BDOS_LIST        5
#define BDOS_RAWIO       6
#define BDOS_PRINTSTR    9
#define BDOS_READSTR     10
#define BDOS_CONST       11
#define BDOS_VERSION     12
#define BDOS_RESET       13
#define BDOS_GETDRV      25
#define BDOS_SETDMA      26
#define BDOS_USER        32

#define bdos( x, y ) bdos_cpm( (long) x, (long) y )
extern long bdos_cpm();

struct cpm_line {
    unsigned char maxlen;
    unsigned char curlen;
    char text[80];
};

struct CPM3DateTime
{
    uint16_t day;   /* day 1 is 1 January 1978 */
    uint8_t hour;   /* packed bcd (nibbles for each digit) */
    uint8_t minute; /* packed bcd */
    uint8_t second; /* packed bcd. for BDOS 155, not BDOS 105 */
};

#include <stdio.h> /* Included only for verification output */

/* K&R definition: No parameters inside parentheses, types defined below */
is_leap_year(year)
int year;
{
    /* Returns 1 if leap year, 0 otherwise */
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        return 1;
    }
    return 0;
}

int sec_to_date(total_seconds)
long total_seconds; /* Standard 32-bit K&R long integer */
{
    /* All declarations must sit at the absolute top of the block */
    long days;
    long seconds_left_in_day;
    int hour;
    int minute;
    int second;
    int weekday_index;
    int year;
    int month;
    int day;
    int days_in_this_year;
    int i;
    
    /* Static array pointers replace modern const strings */
    static char *days_of_week[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", 
        "Thursday", "Friday", "Saturday"
    };
    
    static int days_in_months[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    /* 1. Extract raw whole days and isolate the remaining seconds */
    days = total_seconds / 86400L;
    seconds_left_in_day = total_seconds % 86400L;

    /* Handle potentially negative remainders if calculating negative bounds */
    if (seconds_left_in_day < 0L) {
        seconds_left_in_day = seconds_left_in_day + 86400L;
        days = days - 1L;
    }

    /* 2. Compute Hours, Minutes, and Seconds from the remainder */
    hour = (int)(seconds_left_in_day / 3600L);
    minute = (int)((seconds_left_in_day % 3600L) / 60L);
    second = (int)(seconds_left_in_day % 60L);

    /* 3. Resolve Weekday (Dec 31, 1977 was a Saturday, index 6) */
    weekday_index = (6 + (int)(days % 7L)) % 7;
    if (weekday_index < 0) {
        weekday_index = weekday_index + 7;
    }

    /* Seed baseline date values for Day 0 (Dec 31, 1977) */
    year = 1977;
    month = 12;
    day = 31;

    /* 4. Handle time advancing forward from the baseline epoch date */
    if (days > 0L) {
        days = days - 1L;   /* Step past Dec 31, 1977 to start of Jan 1, 1978 */
        year = 1978;

        while (1) {
            if (is_leap_year(year)) {
                days_in_this_year = 366;
            } else {
                days_in_this_year = 365;
            }

            if (days >= (long)days_in_this_year) {
                days = days - (long)days_in_this_year;
                year = year + 1;
            } else {
                break;
            }
        }

        /* Adjust February table dynamically based on leap cycle status */
        if (is_leap_year(year)) {
            days_in_months[1] = 29;
        } else {
            days_in_months[1] = 28;
        }

        /* 5. Deduct month day counts to isolate calendar month */
        month = 1;
        for (i = 0; i < 12; i++) {
            if (days >= (long)days_in_months[i]) {
                days = days - (long)days_in_months[i];
                month = month + 1;
            } else {
                break;
            }
        }
        
        day = (int)days + 1;
    }

    /* Print comprehensive output using vintage formatting parameters */
    /* printf("Input Seconds: %ld\n", total_seconds); */
    printf("Date: %02d/%02d/%04d (%s)\n", month, day, year, days_of_week[weekday_index]);
    printf("Time: %02d:%02d:%02d\n\n", hour, minute, second);
}

uint8_t bcd_to_uint8_t( bcd ) uint8_t bcd; { return ( ( bcd >> 4 ) * 10 ) + ( bcd & 0xf ); }

int test_bdos_68k( test_conin ) int test_conin;
{
    long r;
    uint8_t s;
    struct CPM3DateTime dt;
    struct cpm_line line;
    uint32_t sec;
    char *msg = "\r\nBDOS string output works.$";

    printf("CP/M-68K BDOS smoke test\r\n");

    r = bdos(BDOS_VERSION, 0L);
    printf("BDOS version raw: 0x%lx\r\n", r);

    r = bdos(BDOS_GETDRV, 0L);
    printf("Current drive: %c: raw=%ld\r\n", (char)('A' + r), r);

    r = bdos(BDOS_USER, 0xFFL);
    printf("Current user: %ld\r\n", r);

    r = bdos(BDOS_CONST, 0L);
    printf("Console status: %ld\r\n", r);

    bdos(BDOS_PRINTSTR, (long)msg);

    bdos( BDOS_CONOUT, '\r' );
    bdos( BDOS_CONOUT, '\n' );
    bdos( BDOS_CONOUT, 'h' );
    bdos( BDOS_CONOUT, 'i' );
    bdos( BDOS_CONOUT, ' ' );
    bdos( BDOS_CONOUT, 'm' );
    bdos( BDOS_CONOUT, 'u' );
    bdos( BDOS_CONOUT, 'l' );
    bdos( BDOS_CONOUT, 't' );
    bdos( BDOS_CONOUT, 'i' );
    bdos( BDOS_CONOUT, 'v' );
    bdos( BDOS_CONOUT, 'e' );
    bdos( BDOS_CONOUT, 'r' );
    bdos( BDOS_CONOUT, 's' );
    bdos( BDOS_CONOUT, 'e' );
    bdos( BDOS_CONOUT, '\r' );
    bdos( BDOS_CONOUT, '\n' );

    if ( test_conin )
    {
        bdos( BDOS_PRINTSTR, "Type a line: $" );

        line.maxlen = sizeof(line.text) - 1;
        line.curlen = 0;
        line.text[0] = '\0';
    
        bdos(BDOS_READSTR, (long)&line);
        line.text[line.curlen] = '\0';
    
        printf("\r\nRead %u chars: [%s]\r\n",
               (unsigned)line.curlen,
               line.text);
    }

    /*
        if running on CP/M 3.0 or later variants, bdos 105 works.
        don't check the cp/m version because some emulators claim 2.2 yet implement bdos 105.
    */

    dt.day = 2; /* if bdos 105 isn't supported, this value won't change. */
    r = bdos( 105, & dt );
    s = bcd_to_uint8_t( (uint8_t) r );
    if ( 2 != dt.day ) /* Is it January 2, 1978? Probably not. */
    {
        sec = (uint32_t) dt.day * (uint32_t) 24 * (uint32_t) 60 * (uint32_t) 60;
        sec += (uint32_t) bcd_to_uint8_t( dt.hour ) * (uint32_t) 60 * (uint32_t) 60;
        sec += (uint32_t) bcd_to_uint8_t( dt.minute ) * (uint32_t) 60;
        sec += s;

#if 0 /* mostly for debugging */       
        printf( "seconds since the start of December 31, 1977: %lu\n", sec );

        printf( "day:  %u\n", dt.day );
        printf( "hour: %u\n", bcd_to_uint8_t( dt.hour ) );
        printf( "minute: %u\n", bcd_to_uint8_t( dt.minute ) );
        printf( "seconds: %u\n", s );
#endif        

        sec_to_date( sec );
    }

    printf("BDOS smoke test done\r\n");
}

int main( argc, argv ) int argc; char * argv[];
{
    test_bdos_68k( argc > 1 );
    return 0;
}
