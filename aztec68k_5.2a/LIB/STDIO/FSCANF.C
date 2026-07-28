/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

#include <stdio.h>
#include <stdarg.h>

int
_gchar(int c, FILE *fp, int unget)
{
	int scanlast;
	if (!unget) {
		if (feof(fp))
			scanlast = EOF;
		else
			scanlast = getc(fp);
	} else
		scanlast = ungetc(scanlast, fp);
	return scanlast;
}

int
fscanf(FILE *stream, const char *format, ...)
{
	register va_list vargs;
	register int ret;

	va_start(vargs, format);
	ret = _scan(stream, _gchar, format, vargs);
	va_end(vargs);
	return(ret);
}

