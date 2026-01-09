/* Copyright (c) 1986 by Manx Software Systems */
#include <FCntl.h>

char *
mktemp(template)
char *template;
{
	register char *cp;
	register unsigned val;
	extern unsigned _Dorg;
	int file;

	cp = template;
	cp += strlen(cp);
	for (val = _Dorg ; ; )
		if (*--cp == 'X') {
			*cp = val%10 + '0';
			val /= 10;
		} else if (*cp != '.')
			break;

	if (*++cp != 0) {
		*cp = 'A';
		while ((file = open(template, O_RDONLY)) >= 0) {
			close(file);
			if (*cp == 'Z') {
				*template = 0;
				break;
			}
			++*cp;
		}
	} else {
		if ((file = open(template, O_RDONLY)) >= 0) {
			close(file);
			*template = 0;
		}
	}
	return template;
}
