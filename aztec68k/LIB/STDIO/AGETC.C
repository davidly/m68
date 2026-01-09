/* Copyright (C) 1981,1982 by Manx Software Systems */
#include "stdio.h"

agetc(ptr)
register FILE *ptr;
{
	register int c;

top:
	if ((c = getc(ptr)) != EOF) {
		switch (c) {
		case 0x04:
			--ptr->_bp;
			ptr->_flags |= _EOF;
			return EOF;
#ifdef MACINTOSH
		case '\r':
			return('\n');
#endif
		case 0:
			goto top;
		}
	}
	return c;
}

