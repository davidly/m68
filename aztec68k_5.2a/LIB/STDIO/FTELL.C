/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

/*
 *	Synopsis
 *
 *	long int ftell(FILE *stream);
 *
 *
 *	Description
 *
 *		The ftell function obtains the current value of the file position
 *	indicator for the stream pointed to by stream. For a binary stream, the
 *	value is the number of characters from the beginning of the file. For a
 *	text stream, its file position indicator contains unspecified information,
 *	usable by the fseek function for returning the file position indicator for
 *	the stream to its position at the time of the ftell call; the difference
 *	between two such return values is not necessarily a meaningful measure of
 *	the number of characters written or read.
 *
 *
 *	Returns
 *
 *		If successful, the ftell function returns the current value of the file
 *	position indicator for the stream. On failure, the ftell function returns
 *	-1L and stores an implementation-defined positive value in errno.
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

long int
ftell(register FILE *stream)
{
	register long pos;

	if ((pos = _lseek(fileno(stream), 0L, SEEK_CUR)) >= 0) {
		if (stream->_flags & _IODIRTY)
			pos += stream->_bp - stream->_buff;
		else if (stream->_bp)
			pos -= stream->_bend - stream->_bp;
		return(pos);
	}
	errno = EINVAL;
	return(EOF);
}

