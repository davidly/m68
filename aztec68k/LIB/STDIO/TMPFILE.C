/* Copyright (C) 1986 by Manx Software Systems, Inc. */

#include <stdio.h>

/* returns a pointer for a temp file which is automatically deleted
	when the program exits; the file is opened for update */

FILE * 
tmpfile ()
{
	register char *cp;
	register FILE *fp;
	char *tmpnam(), *malloc();

	cp = tmpnam (NULL);
	if ( (fp = fopen (cp, "w+")) == NULL )  
		perror (cp);
	else {
		if ((fp->_tmpname = malloc(strlen(cp)+1)) == NULL) {
			fclose(fp);
			unlink(cp);
			return NULL;
		}
		strcpy(fp->_tmpname,cp);
		fp->_flags |= _TEMP;
	}
	return fp;
}

