/* Copyright (C) 1984 by Manx Software Systems */
#include <utime.h>

char *
ctime(clock)
long *clock;
{
	struct tm *tm;

	tm = localtime(clock);
	return asctime(tm);
}
