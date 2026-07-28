/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

/*
 *	Synopsis
 *
 *	int _flsbuf(FILE *stream, int data);
 *
 *
 *	Description
 *
 *		The _flsbuf function flushes the specified stream if it has been opened
 *	for writing or updating and is dirty. If data is not a -1, then data is
 *	placed in the newly flushed buffer and the buffer marked as dirty.
 *	Otherwise, the stream is placed in the neutral state awaiting either reads
 *	or writes.
 *
 *
 *	Returns
 *
 *		The _flsbuf function returns the value of the data passed in. If an
 *	error occurs writing the stream, the error flag is set in the stream and
 *	EOF is returned. If the data value is -1, a zero is returned if nor errors
 *	occur.
 */

#include <stdio.h>
#include <fcntl.h>

extern void (*_close_stdio)(void);

static void
closeall(void)		/* called by exit to close any open files */
{
	register FILE *stream;

	for (stream=_iob;stream<_iob+FOPEN_MAX;stream++)
		fclose(stream);
}

int
_flsbuf(register FILE *stream, register int data)
{
	register short flags;
	register size_t len;
	register unsigned char c;

	if (stream == 0 || (flags=stream->_flags) == 0 || (flags & _IOR) ||
															(flags & _IOSTRNG))
		goto errout;

	stream->_flags &= ~(_IOEOF|_IOUNG);

	if (stream->_buff == 0) {
		if (data == -1)
			return(0);
		_getbuf(stream);
		flags = stream->_flags;
	}

	if ((flags & _IODIRTY) == 0) {
		if (stream->_bp > stream->_buff)	/* unread data in buffer */
			lseek((int)stream->_unit, (long)(stream->_bp-stream->_bend), 1);
		stream->_bp = stream->_buff;
		stream->_bend = stream->_bp + stream->_buflen;
	}
	if (data == -1)
		c = 0;
	else
		c = data;

	len = stream->_bp - stream->_buff;
	if (flags & (_IOLBF|_IONBF)) {
		if (data != -1) {						/* not flushing, add to buf */
			*stream->_bp++ = c;
			flags = stream->_flags |= _IODIRTY;
			_close_stdio = closeall;
			len++;
		}
		if (data == -1 || c == '\n' || len >= stream->_buflen)
			data = -1;
		else {
			stream->_bend = stream->_bp;
			return(c);
		}
	}

	if (flags & _IODIRTY) {
		if (len && _write((int)stream->_unit, stream->_buff, len) != len)
			goto ioerr;
		stream->_flags &= ~_IODIRTY;
	}

	if (data == -1) {						/* flush only and return */
		stream->_bend = stream->_bp = stream->_buff;
		return(c);
	}

	_close_stdio = closeall;
	stream->_flags |= _IODIRTY;
	stream->_bp = stream->_buff;
	stream->_bend = stream->_bp + stream->_buflen;
	return(*stream->_bp++ = c);

ioerr:
	stream->_flags |= _IOERR;
	stream->_bp = stream->_bend = stream->_buff;

errout:
	return(EOF);
}

