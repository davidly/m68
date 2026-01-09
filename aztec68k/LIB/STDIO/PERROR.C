/* Copyright (C) 1986 by Manx Software Systems, Inc. */

#include <stdio.h>
#include <errno.h>

perror (s)
char *s;
{
	if (errno < 0 || errno > sys_nerr)
		return -1;
	if (s)
		fprintf (stderr, "%s: ", s);
	fprintf (stderr, "%s\n", sys_errlist[errno]);
	return 0;
}

