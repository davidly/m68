; Start-up routine for cpm68k program, loaded in macintosh environment

		far	data
		dseg
		public	__mbot,__mtop,__mcur	;*jd 15 Dec 86
		public	__sbot
		public	_errno
		public	_connum,_a5
		public	_cls_

HEAPSIZE	equ	$800	; size of heap space		*jd 15 Dec 86
__mbot	ds.l	1		; ptr to bottom of heap		*jd 15 Dec 86
__mtop	ds.l	1		; ptr to top of heap		*jd 15 Dec 86
__mcur	ds.l	1		; ptr to top of allocated heap space	*jd 15 Dec 86
__sbot	ds.l	1		; ptr to bottom of stack area		*jd 16 Dec 86
_errno	ds.w	1		; place to store error numbers
_connum	ds.w	1		; loader's number for console
_a5		ds.l	1		;caller's a5
_a6		ds.l	1
_a7		ds.l	1
_cls_	ds.l	1
saveret	ds.l	1

		public __H1_org,__H1_end	;*jd 27 Oct 86
		public __H2_org,__H2_end	;*jd 27 Oct 86
		public __H0_org,__H0_end	;*jd 27 Oct 86

		cseg

		public	_Storg_,STKSIZ		
		public	.begin
		entry	.begin
		public	_main
		public	_exit,__exit

.begin:
	move.l	a5,_a5
	move.l	#__H1_org+32766,a5	;set small model reg for cpm prog
	move.l	a6,_a6
	move.w	10(sp),_connum	;save loader's console number
	move.l	0(a7)+,saveret	;save return addr
	move.l	a7,_a7			;save stack pointer

; Calculate length of BSS segment and initialize address register
		move.l	#__H2_end,d0		;*jd 27 Oct 86
		sub.l	#__H2_org,d0		;*jd 27 Oct 86
		beq		.2end				; skip if no BSS segment
		lsr.l	#1,d0				; divide length by 2 and clear words
		move.l	#__H2_org,a0		; BSS base address

; Initialize BSS segment to 0
.2
		clr.w	(a0)+
		dbra	d0,.2
.2end

; Initialize heap ptrs
		move.l	#_Storg_,d0
		add.l	#3,d0			; align heap on 32-bit boundary *jd 15 Dec 86
		and.l	#-8,d0			
		move.l	d0,__mbot	
		move.l	d0,__mcur
		add.l	#HEAPSIZE,d0	
		move.l	d0,__mtop	
; Call cpm program
		jsr		_main	
; Return to loader
_exit
__exit
		move.l	_a5,a5
		move.l	_a6,a6
		move.l	_a7,a7
		move.l	saveret,a0
		jmp	(a0)				;return to loader
		end
