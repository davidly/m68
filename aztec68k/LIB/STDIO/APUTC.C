/* Copyright (C) 1981,1982 by Manx Software Systems */
#include "stdio.h"

aputc(c,ptr)
register int c; FILE *ptr;
{
#if MACINTOSH
	if (c == '\n')
		c = '\r';
#endif
	return putc(c,ptr);
}

