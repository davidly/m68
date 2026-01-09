/* Copyright (C) 1984 by Manx Software Systems */
#include <utime.h>

char *
asctime(tm)
struct tm *tm;
{
	static char days[7][4] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
	static char months[12][4] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	static char buffer[26];

	sprintf(buffer, "%s %s %2d %02d:%02d:%02d %4d\n",
		days[tm->tm_wday], months[tm->tm_mon], tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_year+1900);
	return buffer;
}
