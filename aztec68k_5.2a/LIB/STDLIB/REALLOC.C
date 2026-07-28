/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

/*
 *	Synopsis
 *
 *	void *realloc(void *ptr, size_t size);
 *
 *
 *	Description
 *
 *		The realloc function changes the size of the object pointed to by ptr
 *	to the size specified by size. The contents of the object shall be
 *	unchanged up to the lesser of the new and old sizes. If the new size is
 *	larger, the value of the newly allocated portion of the object is
 *	indeterminate. If ptr is a null pointer, the realloc function behaves like
 *	the malloc function for the specified size. Otherwise, if ptr does not
 *	match a pointer earlier returned by the calloc, malloc, or realloc
 *	function, or if the space has been deallocated by a call to the free or
 *	realloc function, the behavior is undefined. If the space cannot be
 *	allocated, the object pointed to by ptr is unchanged. If size is zero and
 *	ptr is not a null pointer, the object it points to is freed.
 *
 *
 *	Returns
 *
 *		The realloc function returns either a null pointer or a pointer to the
 *	possibly moved allocated space.
 */

#include <stdlib.h>
#include <string.h>

struct mem {
	struct mem *next;
	long size;
};

extern struct mem *_Free;

void *realloc(register void *ptr, register size_t size)
{
	register struct mem *lp, *tp;
	register void *newptr = 0;

	if (ptr) {
		tp = (struct mem *)ptr - 1;
		for (lp=_Free; lp; lp=lp->next)
			if (lp == tp)
				break;
		if (lp == 0)
			return(0);
	}

	if (size && (newptr = malloc(size)) == 0)
		return(0);

	if (ptr) {
		if (size) {
			if (size > lp->size)
				size = lp->size;
			memmove(newptr, ptr, size);
		}
		free(ptr);
	}
    return(newptr);
}

