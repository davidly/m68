/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

/*
 *	Synopsis
 *
 *	void (*signal(int sig, void (*func)(int)))(int);
 *
 *
 *	Description
 *
 *		The signal function chooses one of three ways in which receipt of the
 *	signal number sig is to be subsequently handled. If the value of func is
 *	SIG_DFL, default handling for that signal will occur. If the value of func
 *	is SIG_IGN, the signal will be ignored. Otherwise, func shall point to a
 *	function to be called when that signal occurs. Such a function is called a
 *	signal handler.
 *
 *		When a signal occurs, if func points to a function, first the
 *	equivalent of signal(sig, SIG_DFL); is executed or an implementation-
 *	defined blocking of the signal is performed. (If the value of sig is
 *	SIGILL, whether the reset to SIG_DFL occurs is implementation-defined.)
 *	Next, the equivalent of (*func)(sig); is executed. The function func may
 *	terminate by executing a return statement or by calling the abort, exit, or
 *	longjump function. If func executes a return statement and the value of sig
 *	was SIGFPE or any other implementation-defined value corresponding to a
 *	computational exception, the behavior is undefined. Otherwise, the program
 *	will resume execution at the point it was interrupted.
 *
 *		If the signal occurs other than as the result of calling the abort or
 *	raise function, the behavior is undefined if the signal handler calls any
 *	function in the standard library other than the signal function itself or
 *	refers to any object with static storage duration other than by assigning a
 *	value to a static storage duration variable of type volatile sig_atomic_t.
 *	Furthermore, if such a call to the signal function results in a SIG_ERR
 *	return, the value of errno is indeterminate.
 *
 *		At program startup, the equivalent of 
 *
 *			signal(sig, SIG_IGN);
 *
 *	may be executed for some signals selected in an implementation-defined
 *	manner; the equivalent of
 *
 *			signal(sig, SIG_DFL);
 *
 *	is executed for all other signals defined by the implementation.
 *
 *		The implementation shall behave as if no library function calls the
 *	signal function.
 *
 *
 *	Returns
 *
 *		If the request can be honored, the signal function returns the value of
 *	func for the most recent call to signal for the specified signal sig.
 *	Otherwise, a value of SIG_ERR is returned and a positive value is store in
 *	errno.
 */

#include <signal.h>
#include <errno.h>
#include <fcntl.h>

void (*_sigfuns[_NUMSIG])(int);
static char setup;
static void _sig_setup(void);

void (*signal(register int sig, void (*func)(int)))(int)
{
	register void (*retval)(int);

	if (!setup) {
		_sig_setup();
		setup = 1;
	}
	if ((sig -= _FSTSIG) < 0 || sig >= _NUMSIG) {
		errno = EINVAL;
		return SIG_ERR;
	}
	retval = _sigfuns[sig];
	_sigfuns[sig] = func;
	return retval;
}

int
raise(int sig)
{
	register void (*handler)(int);
	register int tsig = sig - _FSTSIG;

	if (tsig < 0 || tsig >= _NUMSIG)
		return(-1);
	if ((handler = _sigfuns[tsig]) == SIG_DFL)
		_exit(255);
	if (handler != SIG_IGN) {
		_sigfuns[tsig] = SIG_DFL;
		(*handler)(sig);
	}
	return(0);
}

#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <functions.h>

extern int _trapintercept;
extern void *_oldtrap, **_trapaddr;

static void
_sig_setup(void)
{
	struct Task *tp;
	extern struct ExecBase *SysBase;

	tp = FindTask(0L);
	_trapaddr = (void *)&tp->tc_TrapCode;
	_oldtrap= tp->tc_TrapCode;
	tp->tc_TrapCode = (APTR)&_trapintercept;
	if (SysBase->AttnFlags & AFF_68881) {
#asm
		mc68881

		fmove.l	FPCR,d0
		or.w	#$1400,d0
		fmove.l	d0,FPCR
#endasm
	}
}

void
_traphand(long code)
{
	if (code == 5 || code == 7 || code == 50 || code == 53)
		raise(SIGFPE);
	else if (code == 2 || code == 3)
		raise(SIGSEGV);
	else if (code == 4)
		raise(SIGILL);
}

#asm
	far		code
	public	_geta4

	public	__trapintercept
__trapintercept:
	movem.l	d0/a4,-(sp)		;need some registers
	jsr		_geta4			;set up a4
	move.l	8(sp),d0		;get trap number
	cmp.w	#50,d0			;is it 881 div0?
	beq		2$				;yes, do it
	cmp.w	#53,d0			;is it 881 overflow
	beq		2$				;yes, do it
	cmp.w	#7,d0			;is it in right range?
	bgt		1$				;no, do default
	cmp.w	#6,d0			;is it CHK
	bne		2$				;no, go handle it
1$:
	move.l	4(sp),d0		;get a4 for later
	move.l	__oldtrap,4(sp)	;get regular handler
	move.l	d0,a4			;set real a4 value
	move.l	(sp)+,d0		;restore d0 contents
	rts						;jump to it

2$:
	move.l	a0,-(sp)		;save a0
	move.l	usp,a0			;get user stack pointer
	move.l	18(sp),-(a0)	;save old pc
	move.w	14(sp),-(a0)	;save old sr
	movem.l	d0-d7/a0-a6,-(a0)	;save old registers
	movem.l	(sp)+,d1/d2/d3	;restore d0/a0/a4
	move.l	d1,(a0)			;set d0
	move.l	d2,32(a0)		;set a0
	move.l	d3,48(a0)		;set a4
	move.l	d0,-(a0)		;pass trap number on stack
	lea		3$,a1			;get return in case they want to continue
	move.l	a1,-(a0)		;set return address
	move.l	a0,usp			;set new user stack pointer
	lea		__traphand,a0	;get trap handler
	move.l	a0,6(sp)		;modify rte address
	add.w	#4,sp			;remove trap #
	rte						;go and do the handler

3$:
	add.w	#4,sp			;pop arg
	movem.l	(sp)+,d0-d7/a0-a6	;restore old registers
	move.w	(sp)+,ccr		;restore old condition codes
	rts						;and continue where we left off

#endasm

