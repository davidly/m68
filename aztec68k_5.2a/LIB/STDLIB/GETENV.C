/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

/*
 *	Synopsis
 *
 *	char *getenv(const char *name);
 *
 *
 *	Description
 *
 *		The getenv function searches an "environment list", provided by the
 *	host environment, for a string that matches the string pointed to by name.
 *	The set of environment names and the method for altering the environment
 *	list are implementation-defined.
 *
 *		The implementation shall behave as if no library function calls the
 *	getenv function.
 *
 *
 *	Returns
 *
 *		The getenv function returns a pointer to a string associated with the
 *	matched list member. The array pointed to shall not be modified by the
 *	program, but may be overwritten by a subsequent call to the getenv
 *	function. If the specified name cannot be found, a null pointer is
 *	returned.
 */

#if MCH_AMIGA
#include <stdio.h>
#include <string.h>

#define BUFSIZE 256

char *
getenv(const char *str)
{
	FILE *fp;
	char *cp;
	static char buf[BUFSIZE+1];

	if (strlen(str) > BUFSIZE - 4)
		return(0);
	strcpy(buf, "ENV:");
	strcat(buf, str);

	if ((fp = fopen(buf, "r")) == NULL)
		return(0);
	for (cp=buf; cp<buf+BUFSIZE; cp++) {
		if ((*cp = getc(fp)) == EOF)
			break;
	}
	fclose(fp);
	*cp = 0;
	return(buf);
}
#elif MCH_MACINTOSH
#include	"shell.h"
#include	"string.h"
#include	"stdlib.h"

#asm
	public	.getenv
.getenv:
#endasm

static
char *
getenv(const char *name)
{
	register size_t i;
	register char *cp;
	static char *buf;
	char *index();
	void *malloc();

	cp = *Sp->env;
	i = strlen(name);
	while (*cp) {
		if (cp[i] == '=' && strncmp(cp, name, i) == 0) {
			cp += i + 1;
			if (buf)
				free(buf);
			buf = malloc(strlen(cp)+1);
			if (buf)
				strcpy(buf, cp);
			return(buf);
		}
		cp += strlen(cp) + 1;
	}
	return((char *)0);
}

#elif MCH_ROM
char *
getenv(const char *str)
{
	return (char *)0;
}
#endif
