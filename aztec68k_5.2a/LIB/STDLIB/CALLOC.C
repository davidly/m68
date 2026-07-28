/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

/*
 *	Synopsis
 *
 *	void *calloc(size_t nmemb, size_t size);
 *
 *
 *	Description
 *
 *		The calloc function allocates space for an array of nmemb objects, each
 *	of whose size is size. The space is initialized to all bits zero.
 *
 *
 *	Returns
 *
 *		The calloc function returns either a null pointer or a pointer to the
 *	allocated space.
 */

#include <stdlib.h>
#include <string.h>

void * calloc(size_t nelem, size_t size)
{
	register void *cp;
	register size_t s;

	s = nelem * size;
	if (cp = malloc(s))
		memset(cp, 0, s);
	return(cp);
}
