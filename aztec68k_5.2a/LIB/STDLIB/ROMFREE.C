/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */
#include <stdlib.h>

typedef struct mem {
	struct mem * next;
	long size;
} FREE;

FREE *_Free;		/* list of malloc'ed but not free'ed blocks */
FREE *_mlast;	/* ptr to last free'ed block */

void
free(void *area)
{
	register FREE *tp, *hole, *xp;

	if (area == 0)
		return;
	hole = (FREE *)((char *)area - sizeof(FREE));

	/* Remove area from "malloc'ed but not free'ed list": */
	xp = 0;
	for (tp=_Free;tp;tp=tp->next) {
		if (tp+1 == area)
			goto found;
		xp = tp;
	}
	return;
found:
	if (xp)
		hole->next = tp->next;
	else
		_Free = tp->next;

	/* add area to free list */
	for (tp = _mlast ; tp > hole || hole > tp->next ; tp = tp->next) {
		if (tp >= tp->next && (hole > tp || hole < tp->next))
			break;
	}
	hole->next = tp->next;
	tp->next = hole;
	_mlast = tp;
}
