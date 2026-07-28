/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

/*
 *	Synopsis
 *
 *	size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
 *
 *
 *	Description
 *
 *		The fwrite function writes, from the array pointed to by ptr, up to
 *	nmemb members whose size is specified by size, to the stream pointed to by
 *	stream. The file position indicator for the stream (if defined) is advanced
 *	by the number of characters successfully written. If an error occurs, the
 *	resulting value of the file position indicator for the stream is
 *	indeterminate.
 *
 *
 *	Returns
 *
 *		The fwrite function returns the number of members successfully written,
 *	which will be less than nmemb only if a write error is encountered.
 */

#include <stdio.h>

size_t
fwrite(register const void *ptr, register size_t size, register size_t nmemb,
														register FILE *stream)
{
	register size_t total, i;

	for ( total = 0 ; total < nmemb ; ++total ) {
		for ( i = size ; i ; --i ) {
			if ( putc(*(char *)ptr, stream) == EOF )
				goto eof;
			++((char *)ptr);
		}
	}
eof:
	return(total);
}

