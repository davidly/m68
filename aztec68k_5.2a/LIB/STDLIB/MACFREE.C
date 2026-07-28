/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

/*
 *	Synopsis
 *
 *	void free(void *ptr);
 *
 *
 *	Description
 *
 *		The free function causes the space pointed to by ptr to be deallocated,
 *	that is, made available for further allocation. If ptr is a null pointer,
 *	action occurs. Otherwise, if the argument does not match a pointer earlier
 *	returned by the calloc, malloc or realloc function, or if the space has
 *	been deallocated by a call to free or realloc, the behavior is undefined.
 *
 *
 *	Returns
 *
 *		The free function returns no value.
 */

#include <stdlib.h>

void _free(void *area);

void free(void *area)
{
	_free(area);
}
