/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

/*
 *	Synopsis
 *
 *	size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
 *
 *
 *	Description
 *
 *		The fread function reads, into the array pointed to by ptr, up to nmemb
 *	members whose size is specified by size, from the stream pointed to by
 *	stream. The file position indicator for the stream (if defined) is advanced
 *	by the number of characters successfully read. If an error occurs, the
 *	resulting value of the file position indicator for the stream is
 *	indeterminate. If a partial member is read, its value is indeterminate.
 *
 *
 *	Returns
 *
 *		The fread function returns the number of members successfully read,
 *	which may be less than nmemb if a read error or end-of-file is encountered.
 *	If size or nmemb is zero, fread returns zero and the contents of the array
 *	and the state of the stream remain unchanged.
 */

#include <stdio.h>

size_t
fread(register void *ptr, register size_t size, register size_t nmemb,
														register FILE *stream)
{
	register size_t total, i;
	register int c;

	if (size == 0)
		return(0);
	for ( total = 0 ; total < nmemb ; ++total ) {
		for ( i = size ; i ; --i ) {
			if ( (c = getc(stream)) == EOF )
				goto eof;
			*(char *)ptr = (char)c;
			((char *)ptr)++;
		}
	}
eof:
	return(total);
}

