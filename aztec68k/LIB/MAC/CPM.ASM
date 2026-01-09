; Start-up routine for cpm68k program, loaded in macintosh environment

		dseg
		public	__mbot,__mtop,__mcur	;*jd 15 Dec 86
		public	__sbot
		public	_errno
		public	_connum

HEAPSIZE	equ	$800	; size of heap space		*jd 15 Dec 86
__mbot	ds.l	1		; ptr to bottom of heap		*jd 15 Dec 86
__mtop	ds.l	1		; ptr to top of heap		*jd 15 Dec 86
__mcur	ds.l	1		; ptr to top of allocated heap space	*jd 15 Dec 86
__sbot	ds.l	1		; ptr to bottom of stack area		*jd 16 Dec 86
_errno	ds.w	1		; place to store error numbers
_connum	ds.w	1		; loader's number for console

		public __H1_org,__H1_end	;*jd 27 Oct 86
		public __H2_org,__H2_end	;*jd 27 Oct 86
		public __H0_org,__H0_end	;*jd 27 Oct 86

		cseg

		public	_Storg_,STKSIZ		
		public	.begin
		public	_main

.begin:
	move.w	6(sp),_connum	;save loader's console number
;
; Assume program uses small code &/or small data 
; and that it uses A5 to access jmptabs and data
; ...if program uses large code and large data, 
;    and you want to use A5 for other purposes (eg, register variable),
;    delete the next line of code
; ...if it uses a different address register, change the next line
		move.l	#__H1_org+32766,a5	;*jd 17 Nov 86

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
		and.l	#-8,d0			; *jd 15 Dec 86
		move.l	d0,__mbot		; *jd 15 Dec 86
		move.l	d0,__mcur		; *jd 15 Dec 86
		add.l	#HEAPSIZE,d0	; *jd 15 Dec 86
		move.l	d0,__mtop		; *jd 15 Dec 86

; Initialize stack pointers
		move.l	#_Storg_,d0
		sub.l	#STKSIZ,d0		;*jd10/27/86
		move.l	d0,__sbot			; bottom of stack

		jsr		_main				; *jd 10/28/86
		rts						;return to loader
		end
