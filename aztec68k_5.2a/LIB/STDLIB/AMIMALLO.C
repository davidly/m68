/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

/*
 *	Synopsis
 *
 *	void *malloc(size_t size);
 *
 *
 *	Description
 *
 *		The malloc function allocates space for an object whose size is
 *	specified by size and whose value is indeterminate.
 *
 *
 *	Returns
 *
 *		The malloc function returns either a null pointer or a pointer to the
 *	allocated space.
 */

#include <stdlib.h>
#include <functions.h>

struct mem {
	struct mem *next;
	long size;
};

struct mem *_Free;

static void
cleanup(void)
{
	register struct mem *mp, *xp;

	for (mp=_Free;mp;mp=xp) {
		xp = mp->next;
		FreeMem(mp, mp->size+sizeof(struct mem));
	}
	_Free = 0;
}

void *
malloc(size_t size)
{
	register struct mem *ptr;
	extern void (*_cln)(void);

	if (size == 0)
		return(0);
	if ((ptr = AllocMem(size+sizeof(struct mem), 0L)) == 0)
		return((void *)0);
	_cln = cleanup;
	ptr->next = _Free;
	ptr->size = size;
	_Free = ptr;
	return((char *)ptr + sizeof(struct mem));
}

