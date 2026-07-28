/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

#include <stdio.h>
#include <stdarg.h>

int
fprintf(FILE *stream, const char *format, ...)
{
	register va_list vargs;
	register int ret;

	va_start(vargs, format);
	ret = _format(stream, putc, format, vargs);
	va_end(vargs);
	return(ret);
}

