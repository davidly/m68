/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

#include <stdio.h>
#include <stdarg.h>

int
vprintf(const char *format, va_list vargs)
{
	return(_format(stdout, putc, format, vargs));
}

