/* Copyright (c) 1986 by Manx Software Systems */
#include <utime.h>
#include <OSUtils.h>

struct tm *
gmtime(clock)
long *clock;
{
	struct tm *localtime();

	return localtime(clock);
}

struct tm *
localtime(clock)
long *clock;
{
	static struct tm tm;
	struct DateTimeRec dtr;
	static int days[12] = {
		-1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333
	};

	Secs2Date(*clock, &dtr);

	tm.tm_sec = dtr.second;
	tm.tm_min = dtr.minute;
	tm.tm_hour = dtr.hour;
	tm.tm_mday = dtr.day;
	tm.tm_mon = dtr.month - 1; /* 0-11 */
	tm.tm_year = dtr.year - 1900; /* year since 1900 */
	tm.tm_wday = dtr.dayOfWeek - 1; /* 0 = Sunday */
	tm.tm_yday = days[tm.tm_mon] + tm.tm_mday +
					(tm.tm_mon > 1 && (tm.tm_year&3) == 0);
	tm.tm_isdst = 0; /* can't get */
	tm.tm_hsec = 0; /* can't get */

	return(&tm);
}
