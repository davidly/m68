/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

/*
 *	Synopsis
 *
 *	int getw(FILE *stream);
 *
 *
 *	Description
 *
 *		The getw function reads from the input stream pointed to by stream, a
 *	word in an implementation-defined manner. It should be the same as used by
 *	the putw function.
 *
 *
 *	Returns
 *
 *		The getw function returns an int that is the word received. If an error
 *	or end-of-file occurs, EOF is returned. Since EOF is a valid return value,
 *	the ferror and the feof functions must be used to determine if an error
 *	really occurred when EOF is returned.
 */

#include <stdio.h>

int
getw(register FILE *stream)
{
	register int x1,x2;

	if ((x1 = getc(stream)) == EOF || (x2 = getc(stream)) == EOF)
		return(EOF);
	return((x1<<8) | x2);
}

