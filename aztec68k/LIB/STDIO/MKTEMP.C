/* Copyright (C) 1986 by Manx Software Systems, Inc. */

char *
mktemp(template)
char *template;
{
	register char *cp;
	register unsigned long val;
	long _FindTask();

	cp = template;
	cp += strlen(cp);
	for (val = _FindTask(0L) ; ; )
		if (*--cp == 'X') {
			*cp = val%10 + '0';
			val /= 10;
		} else if (*cp != '.')
			break;

	if (*++cp != 0) {
		*cp = 'A';
		while (access(template, 0) == 0) {
			if (*cp == 'Z') {
				*template = 0;
				break;
			}
			++*cp;
		}
	} else {
		if (access(template, 0) == 0)
			*template = 0;
	}
	return template;
}

