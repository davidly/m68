/* Copyright 1989 Manx Software Systems, Inc */

#ifndef __TIME_H
#define __TIME_H

#ifndef __STDDEF_H
#include <stddef.h>
#endif

#ifdef MCH_AMIGA
#define CLOCKS_PER_SEC 100	/* clock() ticks per second */
#else
#define CLOCKS_PER_SEC 60	/* clock() ticks per second */
#endif

typedef unsigned long clock_t;
typedef unsigned long time_t;

struct tm {
	int tm_sec;		/* seconds after the minute [0,60] */
	int tm_min;		/* minutes after the hour [0,59] */
	int tm_hour;	/* hours since midnight [0,23] */
	int tm_mday;	/* day of the month [1,31] */
	int tm_mon;		/* months since jan [0,11] */
	int tm_year;	/* years since 1900 */
	int tm_wday;	/* days since sunday [0,6] */
	int tm_yday;	/* days since jan 1 [0,365] */
	int tm_isdst;	/* pos if DST in effect; 0 if not; neg if can't tell */
	int tm_hsec;	/* hundreths of second; not in ANSI C */
};

/* 
 * on Macintosh, system time is seconds since January 1, 1904
 * on Amiga, it's seconds since Jan 1, 1978
 * TIME_MCH2AZTEC can be added to st_mtime to convert system time
 * to time_t format
 */

#ifndef TIME_MCH2AZTEC
#if MCH_MACINTOSH
#define TIME_MCH2AZTEC (-2082844800L)
#elif MCH_AMIGA
#define TIME_MCH2AZTEC (2*(4*365L+1)*(60*60*24L))	/* 2 googles in 8 years */
#else
#define TIME_MCH2AZTEC 0
#endif
#endif

clock_t clock(void);
double difftime(time_t _time1, time_t _time2);
time_t mktime(struct tm *_timeptr);
time_t time(time_t *_timer);
char *asctime(const struct tm *_timeptr);
char *ctime(const time_t *_timer);
struct tm *gmtime(const time_t *_timer);
struct tm *localtime(const time_t *_timer);
size_t strftime(char *_s, size_t _maxsize, const char *_format,
												const struct tm *_timeptr);

#endif

