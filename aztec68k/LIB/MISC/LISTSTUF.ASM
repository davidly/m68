;:ts=8
	public	_LNextCell
	public	_LAddToCell
	public	_LUpdate
	public	_LSetCell
	public	_LGetCell
	public	_LClick 
	public	_LFind  
	public	_LSize  
	public	_LDelColumn
	public	_LActivate
	public	_LAddRow
	public	_LNew   
	public	_LDoDraw
	public	_LClrCell
	public	_LCellSize
	public	_LAutoScroll
	public	_LDelRow
	public	_LDispose
	public	_LRect  
	public	_LSearch
	public	_LLastClick
	public	_LAddColumn
	public	_LSetSelect
	public	_LScroll
	public	_LGetSelect
	public	_LDraw
;
; Entries expecting Point rather than Point *
	public	_LCELLSIZE
	public	_LCLICK
	public	_LNEW

_LActivate
	move.w	#0,d0
	bra.s	.1
_LAddColumn
	move.w	#4,d0
	bra.s	.1
_LAddRow
	move.w	#8,d0
	bra.s	.1
_LAddToCell
	move.w	#12,d0
	bra.s	.1
_LAutoScroll
	move.w	#16,d0
	bra.s	.1
_LCellSize
	move.l	8(sp),a0
	move.l	(a0),8(sp)
_LCELLSIZE
	move.w	#20,d0
	bra.s	.1
_LClick 
	move.l	10(sp),a0
	move.l	(a0),10(sp)
_LCLICK
	move.w	#24,d0
	bra.s	.1
_LClrCell
	move.w	#28,d0
	bra.s	.1
_LDelColumn
	move.w	#32,d0
	bra.s	.1
_LDelRow
	move.w	#36,d0
	bra.s	.1
_LDispose
	move.w	#40,d0
	bra.s	.1
_LDoDraw
	move.w	#44,d0
	bra.s	.1
_LDraw  
	move.w	#48,d0
	bra.s	.1
_LFind  
	move.w	#52,d0
	bra.s	.1
_LGetCell
	move.w	#56,d0
	bra.s	.1
_LGetSelect
	move.w	#60,d0
	bra.s	.1
_LLastClick
	move.l	#64,d0
	bra.s	.1
_LNew   
	move.l	18(sp),a0
	move.l	(a0),18(sp)
_LNEW
	move.w	#68,d0
	bra.s	.1
_LNextCell
	move.w	#72,d0
	bra.s	.1
_LRect  
	move.w	#76,d0
	bra.s	.1
_LScroll
	move.w	#80,d0
	bra.s	.1
_LSearch
	move.w	#84,d0
	bra.s	.1
_LSetCell
	move.w	#88,d0
	bra.s	.1
_LSetSelect
	move.w	#92,d0
	bra.s	.1
_LSize  
	move.l	#96,d0
	bra.s	.1
_LUpdate
	move.w	#100,d0
.1
	movea.l	(a7)+,a0
	move.w	d0,-(a7)
	move.l	a0,-(a7)
	dc.w	$ade7	;Pack0 + autopop
