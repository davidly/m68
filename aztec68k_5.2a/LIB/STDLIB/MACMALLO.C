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
#include <memory.h>

static void _Clean(void);
static void _Put(Handle );
static void _Get(Handle );



void *
malloc(size_t size)
{
	register Handle h;

	if (size == 0)
		return((void *)0);
	_Clean();
	h = NewHandle(size);
	if (h) {
		MoveHHi(h);
		HLock(h);
		return(*h);
	} else
		return((void *)0);
}

void _free(void *area)
{
	register Handle h;

	if (area) {
		h = RecoverHandle(area);
		_Put(h); /* adds it to a psuedo "free" list */
	}
}

void *_realloc(register void *ptr, register size_t size)
{
	register Handle h;
	register char *rc;

	if (ptr) {
		if (size) {
			h = RecoverHandle(ptr);
			_Get(h); /* removes it from the "free" list and cleans out the
						rest of the list */
			HUnlock(h);
			SetHandleSize(h, (long )size);
			MoveHHi(h);
			rc = (MemError() ? 0L : *h);
			HLock(h);
			return(rc);
		}
		else {
			_free(ptr);
			return((void *)0);
		}
	}
	else
		return(malloc(size));
}


typedef struct freelist {
	struct freelist *f_chain;
	Handle	item;
} FREE;

static FREE _Free;

static void _Put(Handle h)
{
	register FREE *cp;
	extern FREE _Free;

	cp = (FREE *) NewPtr((long )sizeof(FREE));
	cp->f_chain = _Free.f_chain;
	_Free.f_chain = cp;
	cp->item = h;
	HUnlock(h);
}

static void _Get(Handle h)
{
	register FREE *cp, *last;

	/* take it out of the list */
	for (cp = &_Free; cp; last = cp, cp = cp->f_chain)
		if (h == cp->item) {
			last->f_chain = cp->f_chain;
			break;
		}

	_Clean();
}

static void _Clean(void)
{
	register FREE *cp, *xp;

	cp = _Free.f_chain;
	while(cp) {
		DisposHandle(cp->item);
		xp = cp->f_chain;
		DisposPtr((char *)cp);
		cp = xp;
	}
	_Free.f_chain = 0L;
}
