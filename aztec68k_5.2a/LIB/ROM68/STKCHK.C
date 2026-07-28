#include <fcntl.h>

#asm
;:ts=8
; Copyright (C) 1986,1987 by Manx Software Systems, Inc.
;
	dseg
	public	__stkbase,_Storg_
	cseg
	public	__stkover
	public	__stkchk
__stkchk:
	move.l	__stkbase,a0
	cmp.l	a7,a0
	bge	oflow
	cmp.l	#'MANX',4(a0)
	bne	oflow
	rts
oflow
	move.l	#_Storg_,d0	;reinitialize SP near the top of stack area.
	sub.l	#16,d0		;you will probably need to customize this code
	move.l	d0,a7		
	jmp	__stkover
#endasm
/*
 *	_stkover is called when __stkchk detects an
 *	overrun of the stack area.
 *  	It must be customized for use on each ROM system
 */
 void
_stkover(void)
{
	write(1, "Stack overflow!!\n", 17L);
	_exit(100);
}
