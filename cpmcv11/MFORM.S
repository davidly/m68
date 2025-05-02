*****************************************************************
*								*
*	CP/M-68K Memory Disk Formatter				*
*	For CompuPro M-Drive/H					*
*	by Michael Perry					*
*	04May83 - llo						*
*	27Apr83	- mp						*
*	14Jun83 - jrs						*
*								*
*****************************************************************


* Bios calls

seldsk	=	9
settrk	=	10
setsec	=	11
setdma	=	12
read	=	13
write	=	14

* BDOS calls

print	=	9
conin	=	1

* memory disk standard I/O address

mddata	=	$ff00c6		* memory disk data port
mdadr	=	$ff00c7		* memory disk address selection port

nomark	=	0		* zero if we use the marker sector here

	.text

	link	a6,#0		* mark stack frame
	move.l	8(a6),a0	* get the address of the base page
	lea	$80(a0),a4	* get start of command line
	cmp.b	#0,(a4)		* see if anything there
	beq	err1
	move.b	1(a4),d1	* get argument
	cmp.b	#'P',d1		* see if valid CPM
	bgt	grterr		* branch if > "P:"
	move.b	d1,drv		* put drive number in message
	sub.b	#'A',d1		* remove ascii bias
	move.l	#seldsk,d0	* select given drive
	trap	#3
	move.l	d0,a0		* dph address
	move.l	14(a0),a1	* dpb address

	move	(a1),spt	* sectors per track
	subq	#1,spt

	move	8(a1),d7	* directory entries
	addq	#1,d7		* bias to prevent div0 trap
	ror.l	#1,d7		* d7 = # of directory sectors
	and.l	#$ffff,d7	* mask out the unnecessary bits
	divs	(a1),d7		* d7(15-0) = # of directory tracks
	subq	#1,d7
	and	#$1ff,d7
	move	d7,trks

	move	14(a1),track	* first sector (track offset)

	.ifeq	nomark

	clr.l	d0
	move.b	d0,mdadr
	move.b	d0,mdadr
	move.b	d0,mdadr
	cmp.b	#'L',mddata
	bne	dooit
	cmp.b	#'L',mddata
	bne	dooit
	cmp.b	#'O',mddata
	bne	dooit		* not equal so go format

	.endc

* here if the memory drive was already formatted, so we need confirmation

	move.b	3(a4),d0
	or.b	#$20,d0
	cmp.b	#'y',d0		* check if we really need to confirm
	beq	dooit		* I guess not

	move.l	#confrm,d1	* print confirm message
	move	#print,d0
	trap	#2
	move	#conin,d0	* get answer
	trap	#2
	or.b	#$20,d0		* make answer lower case
	cmp.b	#'y',d0		* see if confirmation
	bne	exit		* exit if no confirm

* fill buffer with 128 e5's

dooit:
	move.b	#31,d0
	lea	buffer,a0
fill:
	move.l	#$e5e5e5e5,(a0)+
	dbra	d0,fill

trklp:
	move	spt,secs
	move	#0,sector
seclp:
	move	sector,d1
	move.l	#setsec,d0
	trap	#3
	move	track,d1
	move.l	#settrk,d0
	trap	#3
	move.l	#buffer,d1
	move.l	#setdma,d0
	trap	#3
	move.l	#write,d0
	trap	#3
	addq	#1,sector
	subq	#1,secs
	bge	seclp		* next virtual track
	addq	#1,track
	subq	#1,trks
	bge	trklp		* next virtual sector

	.ifeq	nomark

	clr.l	d0		* no more to do, so set the formatting marker
	move.b	d0,mdadr	* point the counter to track 0, sector 0
	move.b	d0,mdadr
	move.b	d0,mdadr
	move.b	#'L',mddata	* check byte (unused) for track 0, sector 0
	move.b	#'L',mddata	* check byte (unused) for track 1, sector 0
	move.b	#'O',mddata	* check byte (unused) for track 2, sector 0

	.endc

	bra	exit
	rts

grterr:	move.l	#invld,d1
	bra	erprnt

err1:	move.l	#nodrv,d1	* print message

erprnt:	move	#print,d0
	trap	#2

exit:	move	#0,d0		* and exit
	trap	#2
	rts

	.data

spt:	.dc.w	0
track:	.dc.w	0
sector:	.dc.w	0
trks:	.dc.w	0
secs:	.dc.w	0

	.even

invld:	.dc.b	$a,$d,'Invalid drive specified.',$a,$d,'$'
nodrv:	.dc.b	$a,$d,'No drive specified.',$d,$a,'$'
confrm:	.dc.b	$d,$a,'Confirm format on drive '
drv:	.dc.b	'  (Y or N):$'

	.bss
	.even

buffer:	.ds.b	128

	.end
