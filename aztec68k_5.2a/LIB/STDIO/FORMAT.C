/* Copyright (C) 1991 by Manx Software Systems */
#include <stdarg.h>
#include <stdlib.h>

int _format(void *ptr, int (*putsub)(int, void *ptr), 
	const char *format,va_list varg);

int
format(register int (*putsub)(int, void *ptr), const char *fmt, ...)
{
	va_list vargs;
	register int ret;

	va_start (vargs, fmt);
	ret = _format (NULL, putsub, fmt, vargs);
	va_end(vargs);
	return (ret);
}
