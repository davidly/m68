/* Copyright Manx Software Systems, Inc. 1988.  All rights reserved */

#ifndef __ERRNO_H 
#define __ERRNO_H 

extern int errno;

#define ENOENT	1		/* No such file or directory */
#define E2BIG	2		/* Argument list too long */
#define EBADF	3		/* Bad file descriptor */
#define ENOMEM	4		/* Not enough memory */
#define EEXIST	5		/* File (already) exists */
#define EINVAL	6		/* Invalid argument */
#define ENFILE	7		/* Too many open files in the system */
#define EMFILE	8		/* Too many open files in a process */
#define ENOTTY	9		/* Not a console device */
#define EACCES	10		/* Permission denied */
#define EIO		11		/* I/O error (physical, usually) */
#define ENOSPC	12		/* No space left on device */
#define ERANGE	13		/* (math) Result too large */
#define EDOM	14		/* (math) Argument domain error */
#define ENOEXEC	15		/* (f)exec format error */
#define EROFS	16		/* Read-only file system */
#define EXDEV	17		/* Cross-device rename */
#define EAGAIN	18		/* Nothing to read */

#if !__STDC__ /* non ANSI C definitions */

extern char *sys_errlist[];
extern int sys_nerr;

#endif /* !__STDC__ */

#endif

