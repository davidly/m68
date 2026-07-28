/* Copyright 1988 Manx Software Systems, Inc. */

/*
 *	clock_t clock(void);
 *
 *	determines the processor time used.
 *
 *	returns the implementation's best approximation to the processor time
 *	used by the program since the beginning of an implementation-defined
 *	era related only to the program invocation. To determine the time in
 *	seconds, the value returned by the clock function should be divided
 *	by the value of the macro CLOCKS_PER_SEC in time.h. If the processor
 *	time used is not available or its value cannot be represented, the
 *	function returns the value (clock_t)-1.
 */

#include <time.h>

#if MCH_AMIGA
#include <functions.h>
#include <libraries/dos.h>

clock_t
clock(void)
{
	struct DateStamp date;
	clock_t ticks;

	(void) DateStamp (&date);
	ticks = (date.ds_Days * (24*60*60*CLOCKS_PER_SEC) +
			(date.ds_Minute * (60*CLOCKS_PER_SEC)) + date.ds_Tick);
	if (ticks == 0L)
		return((clock_t)-1);
	else
		return (ticks);
}
#elif MCH_MACINTOSH
#include <Events.h>

clock_t
clock(void)
{
	return((clock_t)TickCount());
}

#elif MCH_ROM
clock_t
clock(void)
{
	clock_t x=-1;
	return x;
}
#endif
