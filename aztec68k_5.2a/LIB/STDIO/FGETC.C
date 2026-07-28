/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

/*
 *	Synopsis
 *
 *	int fgetc(FILE *stream);
 *
 *
 *	Description
 *
 *		The fgetc function obtains the next character (if present) as an
 *	unsigned char converted to an int, from the input stream pointed to by
 *	stream, and advances the associated file position indicator for the stream
 *	(if defined).
 *
 *
 *	Returns
 *
 *		The fgetc function returns the next character from the input stream
 *	pointed to by stream. If the stream is at end-of-file, the end-of-file
 *	indicator for the stream is set and fgetc returns EOF. If a read error
 *	occurs, the error indicator for the stream is set and fgetc returns EOF.
 */

#asm
	cseg
	xdef	_fgetc
_fgetc:
	jmp		_getc
#endasm

/*
 *	Synopsis
 *
 *	int getc(FILE *stream);
 *
 *
 *	Description
 *
 *		The getc function is equivalent to fgetc, except that if it is
 *	implemented as a macro, it may evaluate stream more than once, so the
 *	argument should never be an expression with side effects.
 *
 *
 *	Returns
 *
 *		The getc function returns the next character from the input stream
 *	pointed to by stream. If the stream is at end-of-file, the end-of-file
 *	indicator for the stream is set and getc returns EOF. If a read error
 *	occurs, the error indicator for the stream is set and getc returns EOF.
 */

#include <stdio.h>

#undef getc

int
getc(register FILE *stream)
{
	if (stream == 0 || stream->_flags == 0)
		return(EOF);
	if (stream->_bp < stream->_bend)
		return(*stream->_bp++);
	return(_filbuf(stream));
}

