/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

/*
 *	Synopsis
 *
 *	int putw(int w, FILE *stream);
 *
 *
 *	Description
 *
 *		The putw function outputs the word w to the stream pointed to by stream
 *	in an implementation defined manner.
 *
 *
 *	Returns
 *
 *		The putw function returns the argument w as its value and EOF if an
 *	error occurs while writing to the output stream. Since EOF is a valid
 *	argument to the putw function, the caller must use the ferror or the feof
 *	function to determine if an error has really occurred when EOF is returned.
 */

#include <stdio.h>

int
putw(register int w, register FILE *stream)
{
	if (putc(w>>8, stream) < 0) 
		return(EOF);
	else if (putc(w, stream) < 0)
		return(EOF);
	return(w);
}

