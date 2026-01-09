/* Copyright (C) 1981,1982 by Manx Software Systems */
#include "stdio.h"

#undef putchar

putchar(c)
{
	return aputc(c,stdout);
}
