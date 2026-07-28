/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

/*
 *	Synopsis
 *
 *	int fputc(int c, FILE *stream);
 *
 *
 *	Description
 *
 *		The fputc function writes the character specified by c (converted to an
 *	unsigned char) to the output stream pointed to by stream, at the position
 *	indicated by the by the associated file position indicator for the stream
 *	(if defined), and advances the indicator appropriately. If the file cannot
 *	support positioning requests, or if the stream was opened with append mode,
 *	the character is appended to the output stream.
 *
 *
 *	Returns
 *
 *		The fputc function returns the character written. If a write error
 *	occurs, the error indicator for the stream is set and fputc returns EOF.
 */

#asm
	cseg
	xdef	_fputc
_fputc:
	jmp		_putc
#endasm

/*
 *	Synopsis
 *
 *	int putc(int c, FILE *stream);
 *
 *
 *	Description
 *
 *		The putc function is equivalent to fputc, except that if it is
 *	implemented as a macro, it may evaluate stream more than once, so the
 *	argument should never be an expression with side effects.
 *
 *
 *	Returns
 *
 *		The putc function returns the character written. If a write error
 *	occurs, the error indicator for the stream is set and putc returns EOF.
 */

#include <stdio.h>

#undef putc

int
putc(int c, register FILE *fp)
{
	if (fp == 0 || fp->_flags == 0)
		return(EOF);
	if (fp->_bp < fp->_bend)
		return(*fp->_bp++ = c);
	return(_flsbuf(fp, (int)(unsigned char)c));
}

