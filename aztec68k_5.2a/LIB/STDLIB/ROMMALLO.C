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
#include <fcntl.h>

typedef struct mem {
	struct mem	*next;
	long	size;
} FREE;

extern FREE *_Free;	/* list of malloc'ed but not free'ed blocks */
extern FREE *_mlast;	/* ptr to last free'ed block */

void *
malloc(size_t size)
{
	register FREE *_alloc(long), *tp;

	if ((tp = _alloc(size+sizeof(FREE))) == 0)
		return 0;
	tp->next = _Free;
	tp->size = size;
	_Free = tp;
	return (void *)((char *)tp+sizeof(FREE));
}

static FREE	head;
static FREE save;

#define GRAIN	(256*sizeof(FREE))

extern void *sbrk();

FREE *
_alloc(long size)
{
	register FREE *tp, *prev;
	int times;

	size = (size+1) & ~1;
	if (size < sizeof(FREE))
		size = sizeof(FREE);
	if ((prev = _mlast) == NULL)
		_mlast = head.next = prev = &head;

	times = 0;
	for (tp = prev->next ; ; prev = tp, tp = tp->next) {
		/* merge blocks adjacent to the current loc */
		while (tp != tp->next &&
				(FREE *)((char *)tp+tp->size) == tp->next) {
			tp->size += tp->next->size;
			tp->next = tp->next->next;
		}

		if (tp->size >= size) {	/* if we have a big enough block */
			/* check to see if leftover is too small */
			if (tp->size < size+sizeof(FREE))
				/* if too small, give'm all of it */
				prev->next = tp->next;
			else { /* otherwise split the block */
				_mlast = (FREE *)((char *)tp + size);
				prev->next = _mlast;
				save = *_mlast;
				_mlast->next = tp->next;
				_mlast->size = tp->size - size;
				tp->size = size;
				tp->next = 0; /* for free */
			}
			_mlast = prev;
			return (FREE *)tp;
		} 

		/* else, we don't have a big enough block, yet */
		/* have we looked through all the free list? */
		if (tp >= tp->next && ++times > 1) {
			if ((tp = (FREE *)sbrk(GRAIN)) == (FREE *)-1)
#ifdef DEBUG
				{ printf("sbrk failed!\n"); return (FREE *)NULL; }
#else
				return (FREE *)NULL;
#endif
			tp->size = GRAIN;
			tp->next = 0; /* for free */
			/* add new block to "malloc'ed but not free'ed list */
			tp->next = _Free;
			tp->size = size;
			_Free = tp;

			free((char *)tp+sizeof(FREE));
			tp = &head;	/* start at beginning to merge new with old */
		}
	}
}

#ifdef MEMTEST
prtlist()
{
	register FREE *ptr;
	register int i;

	if (_mlast == 0) {
		printf("List is empty\n");
		return;
	}

	ptr = &head;
	i = 0;
	do {
		printf("%c%08lx  ", ptr==_mlast ?'*':' ', ptr);
		if (++i == 10) {
			putchar('\n');
			i = 0;
		}
		ptr = ptr->next;
	} while (ptr != &head);
	if (i)
		putchar('\n');
}
#endif
