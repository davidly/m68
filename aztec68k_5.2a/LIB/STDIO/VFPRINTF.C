/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

#include <stdio.h>
#include <stdarg.h>

#undef putc

int
vfprintf(FILE *stream, const char *format, va_list vargs)
{
	return(_format(stream, putc, format, vargs));
}

