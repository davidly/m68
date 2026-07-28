/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

/*
 *	time_t time(time_t *timer);
 *
 *	determines the current calendar time in seconds since January 1, 1970.
 *
 *	returns the best approximation to the current calendar time. The value
 *	(time_t)-1 is returned if the calendar time is not available. If timer
 *	is not a null pointer, the return value is also assigned to the object
 *	it points to.
 */
#include <time.h>

#if MCH_AMIGA

#include <exec/types.h>
#include <exec/lists.h>
#include <devices/timer.h>
#include <functions.h>

#define SECS70TO78	(2*(4*365L+1)*(60*60*24L))	/* 2 googles in 8 years */

time_t
time(time_t *tt)
{
	struct MsgPort *CreatePort();
	struct timerequest t;
	register time_t hz;

	if (OpenDevice(TIMERNAME, (long)UNIT_VBLANK, &t.tr_node, 0L) == 0) {
		t.tr_node.io_Message.mn_ReplyPort = CreatePort(0L, 0L);
		t.tr_node.io_Command = TR_GETSYSTIME;
		DoIO(&t.tr_node);
		hz = t.tr_time.tv_secs + (t.tr_time.tv_micro + 500000) / 1000000;
		CloseDevice(&t.tr_node);
		DeletePort(t.tr_node.io_Message.mn_ReplyPort);
		hz += TIME_MCH2AZTEC;
	}
	else
		hz = -1;

	if (tt)
		*tt = hz;
	return(hz);
}

#elif MCH_MACINTOSH
#include <OSUtils.h>

#define SECS04TO70  2082844800L

time_t 
time(time_t *tloc)
{
	unsigned long x;

	GetDateTime(&x);
	x += TIME_MCH2AZTEC;

	if (tloc)
		*tloc = x;

	return x;
}

#elif MPU8086
#if 0
#define SECS04TO70	(16*(4*365L+1)*(60*60*24L))
#endif
#define SECS04TO70	2019686400L

time_t 
time(time_t *tloc)
{
	unsigned long x;

	GetDateTime(&x);
	x-=(unsigned long)SECS04TO70;

	if (tloc)
		*tloc = x;

	return x;
}

#elif MCH_ROM

time_t  
time(time_t *tloc) 
{
	time_t x=-1;
	if (tloc)
		*tloc = x;
	return x;
}
#endif
