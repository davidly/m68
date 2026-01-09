/* Copyright (C) 1981,1982 by Manx Software Systems */
#include "stdio.h"

fwrite(buffer,size,number,stream)
register char *buffer; unsigned size,number;
FILE *stream;
{
	register unsigned i, j;

	for ( i = 0 ; i < number ; ++i ) {
		for (j=0;j<size;j++) {
			if ( putc(*buffer++,stream) == EOF )
				return 0;
		}
	}
	return number;
}

