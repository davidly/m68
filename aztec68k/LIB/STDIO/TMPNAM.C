/* Copyright (C) 1986 by Manx Software Systems, Inc. */

#include <stdio.h>

static char work[] = "AAAAA";

char *
tmpnam(s)
char *s;
{
	static char tmpbuf[L_tmpnam];
	register char *cp;

	if (s == NULL)
		s = tmpbuf;
	for (;;) {
		strcpy(s,P_tmpdir);
		strcat(s,work);
		strcat(s,"XXX.XXX");
		for (cp = work ; *cp ; ++cp)
			if (*cp == 'Z')
				*cp = 'A';
			else {
				++*cp;
				break;
			}
		if (mktemp(s))
			break;
	}
	return s;
}
