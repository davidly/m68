; Copyright (C) 1984 by Manx Software Systems, Inc.
;:ts=8
;
;
	far	data
	dseg
savea5	ds.l	1
	public	_a5
	cseg
	public	_PBWrite
_PBWrite
	move.l	a5,savea5
	move.l	_a5,a5
	move.l	4(sp),a0
	move.w	8(sp),d0
	bne.s	.1
	dc.w	$a003
	bra.s	.2
.1
	dc.w	$a403
.2
	move.l	savea5,a5
	rts
;
