/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

/*
 *	Synopsis
 *
 *	char *strerror(int errnum);
 *
 *
 *	Description
 *
 *	    The strerror function maps the error number in errnum to an error
 *	message string.
 *
 *		The implementation shall behave as if no library function calls the
 *	strerror function.
 *
 *
 *	Returns
 *
 *		The strerror function returns a pointer to the string, the contents of
 *	which are implementation-defined. The array pointed to shall not be
 *	modified by the program, but may be overwritten by a subsequent call to the
 *	strerror function.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

char *sys_errlist[] = {
/* 0		*/	"",
/* ENOENT	*/	"No such file or directory",
/* E2BIG	*/	"Arg list too long",
/* EBADF	*/	"Bad file descriptor",
/* ENOMEM	*/	"Not enough memory",
/* EEXIST	*/	"File exists",
/* EINVAL	*/	"Invalid argument",
/* ENFILE	*/	"File table overflow",
/* EMFILE	*/	"Too many open files",
/* ENOTTY	*/	"Not a console",
/* EACCES	*/	"Permission denied",
/* EIO		*/	"I/O error",
/* ENOSPC	*/	"No space left on device",
/* ERANGE	*/	"Result too large",
/* EDOM		*/	"Argument out of domain",
/* ENOEXEC	*/	"Exec format error",
/* EROFS	*/	"Read-only file system",
/* EXDEV	*/	"Cross-device rename",
/* EAGAIN	*/	"Nothing to read",
};

int	sys_nerr = { sizeof(sys_errlist)/sizeof(sys_errlist[0]) };
#define LERRNO EAGAIN

char *
strerror(register int errnum)
{
	if (errnum >= 0 && errnum <= LERRNO)
		return(sys_errlist[errnum]);
	return("Unknown error");
}

