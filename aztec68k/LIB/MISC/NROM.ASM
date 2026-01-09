; Copyright (C) 1986,1987 by Manx Software Systems, Inc.
;:ts=8
;
;
	Public __newrom
__newrom
	move.w #0,d0
	cmp.w #$FFFF,$28E
	beq	.1
	move.w #1,d0
	cmp.w #$7FFF,$28E
	beq .1
	move.w #2,d0
.1
	rts
;
