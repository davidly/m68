/* Copyright (C) 1981,1982 by Manx Software Systems */
#include "stdio.h"

aputc(c,ptr)
register int c; FILE *ptr;
{
	/* if (c == '\n')
		putc('\r',ptr); */
	return putc(c,ptr);
}

