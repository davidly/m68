*****************************************************************
*								*
*	CP/M-68K Floppy Disk Formatter				*
*	for CompuPro CPU-68K and Disk 1 floppy disk controller	*
*	by Michael A. Perry					*
*	last modified 09May83					*
*	Double Density, Double Sided				*
*								*
*****************************************************************

iobase	=	$ff0000		* base of 68000's memory mapped i/o

dstat	=	iobase+$c0	* disk status port
ddata	=	dstat+1		* disk data port
ddma	=	dstat+2		* disk dma port

maxdsk	= 4	* this BIOS supports 4 floppy drives
retries	=	3

* BDOS functions
btype	=	9

* BIOS functions
bqkey	=	2
bkey	=	3
bemit	=	4

* begin formatter

start:
	link	a6,#0
	move.l	#signon,d1	* say hello
	bsr	type

restart:
	move.l	#qdrive,d1	* which drive?
	bsr	type
	bsr	upkey		* get upper-case character
	move.b	d0,d1
	sub.b	#$41,d0		* start from 'A'
	cmp.b	#3,d0
	bhi	restart
	move.b	d0,drive
	bsr	emit		* echo drive

	bsr	sds		* is drive ready and not protected?
	move.b	drvstat,d5
	and	#$20,d5
	bne	ready
	move.l	#nrerr,d1
	bsr	type
	bra	redo
ready:
	move.b	drvstat,d5
	and	#$40,d5
	beq	writeok
	move.l	#wperr,d1
	bsr	type
	bra	redo
writeok:

	move.l	#qdense,d1	* which density?
	bsr	type
	bsr	key
	move.b	d0,d1
	sub.b	#$30,d0
	and	#3,d0
	move.b	d0,dense	* density for all but track 0
	bsr	emit		* echo density

	move.l	#certain,d1	* double check
	bsr	type
	bsr	key
	cmp.b	#$20,d0		* is key a space?
	bne	restart		* retry if not

	bsr	restore		* home the disk
	move.l	#form,d1	* announce start
	bsr	type

	clr.b	track
	clr.b	density		* force zero density for track 0
	move	#76,d7		* number of tracks -1
format:
	move.b	track,d6
	bsr	seek		* move the head
	bsr	format1		* format one side
	tst.b	sides		* is it double sided?
	beq	single
	move.b	#$ff,side	* select side one
	bsr	format1		* format it
	move.b	#0,side		* return to side zero
single:
	move.b	#$46,d1		* print an F
	bsr	emit
	move.b	dense,density	* for remainder of tracks
	add.b	#1,track
	dbra	d7,format	* repeat for all tracks
	bsr	restore		* home the disk

	bsr	cr
	clr.b	track
	clr.b	density		* force zero density for track 0
	move	#76,d7		* number of tracks -1
verify:
	move.b	track,d6
	bsr	seek		* move the head
	bsr	read		* read
	bne	vererr
	move.b	#$56,d1		* print V for verify
	bra	verok
vererr:
	move.b	#$45,d1		* or E for error
verok:
	bsr	emit
	move.b	dense,density	* for remaining tracks
	add.b	#1,track
	dbra	d7,verify	* repeat for all tracks
	bsr	restore		* home the disk

redo:
	move.l	#qagain,d1	* do you want to retry?
	bsr	type
	bsr	upkey
	cmp.b	#$59,d0		* is it 'Y' ?
	beq	restart		* then return to beginning
exit:
	unlk	a6
	rts			* otherwise exit


*
* subroutines
*

key:
	move	#bkey,d0
	trap	#3
	cmp.b	#3,d0		* control C ?
	beq	exit
	rts

emit:
	move	#bemit,d0
	trap	#3
	rts

upkey:
	bsr	key
	and	#$00df,d0	* convert to upper case
	rts

cr:
	move.b	#13,d1
	bsr	emit
	move.b	#10,d1
	bsr	emit
	rts

type:
	move	#btype,d0
	trap	#2
	rts

dot:
* print the number in d7, as two decimal digits
	move.b	#13,d1
	bsr	emit
	clr.l	d2
	move.b	d7,d2
	divu	#10,d2
	or	#$30,d2
	move	d2,d1
	bsr	emit
	swap	d2
	or	#$30,d2
	move	d2,d1
	bsr	emit
	rts

derror:
	bsr	type
	move.l	#drverr,d1
	bsr	type
	move.b	drive,d1
	add	#$41,d1
	bsr	emit
	bsr	cr
	move.l	#qmess,d1
	bsr	type
	bsr	key
	or.b	#$20,d0
	cmp.b	#$72,d0
	bne	redo
	bsr	cr
	rts

send:
* send byte in d4 to disk controller
        btst    #7,dstat
        beq     send
        move.b  d4,ddata
        rts

results:
* read all result bytes from disk controller
        lea   stat,a0
res1:
        btst    #7,dstat
        beq     res1
        btst    #6,dstat
        beq     resx
        move.b  ddata,(a0)+
        bra     res1
resx:   rts

waitint:
* wait until Disk 1 interrupt flag becomes true
	btst	#7,ddma
	beq	waitint
	rts

sis:
* Sense Interrupt Status: must be used after seek or home
	bsr	waitint
        moveq   #8,d4
        bsr     send
        bsr	results		* read all result bytes
        move.b  stat,d4		* status for correct drive?
	move.b	drive,d1
	eor	d4,d1
	and	#3,d1
	bne	sis		* try again if not.
        andi.b  #$f8,d4		* ignore drive number
        cmpi.b  #$20,d4		* successful seek?
	beq	sisx		* exit if ok
	move.b	stat,d4
	andi.b	#$18,d4		* is drive not ready?
	beq	sisx		* exit if ok
	move.l	#nrerr,d1	* else print message
	bsr	derror
sisx:	
        rts

restore:
* home the disk
        moveq   #2,d2		* 3 retries
rest1:
        moveq   #$7,d4
        bsr     send
        move.b  drive,d4
        bsr     send
        bsr     sis		* get status
        dbeq    d2,rest1	* retry on error
        rts

seek:
* position the disk head to the track in d6.b
        moveq   #2,d2		* 3 retries
seek1:
        moveq   #$0f,d4
        bsr     send
        move.b  drive,d4
        bsr     send
        move.b  d6,d4		* track number
        bsr     send
        bsr     sis		* get status
        dbeq    d2,seek1	* retry on error
	beq	seekx		* exit if ok
	bsr	restore		* home disk if fatal
seekx:
        rts

sds:
* Sense Drive Status
* tests for double sided
	move.b	#4,d4
	bsr	send
	move.b	drive,d4
	bsr	send
	bsr	results
	move.b	stat,d5
	move.b	d5,drvstat
	and.b	#8,d5		* select double sided bit
	move.b	d5,sides
	rts

setcmd:
* set up the command buffer for the next read or write
	moveq	#0,d1
	move.l	d1,d2
	tst.b	sides
	beq	setone
	move.b	#$80,d1		* set mt bit of command byte
setone:
	move.b	track,d6
	beq	setcmd1		* force single density on track 0
	move.b	density,d2	* get density byte
	beq	setcmd1		* if not single density
	or.b	#$40,d1		* then set mfm bit
setcmd1:
	or.b	#6,d1
	move.l	d2,d3		* d2 and d3 have density byte = n
	mulu	#4,d3		* index into gap table
	add.l	#gaps,d3
	move.l	d3,a1		* point a1 at gpl/dtl entry
	lea	cmd,a0		* set up the command buffer
	move.b	d1,(a0)+	* command
	move.b	drive,(a0)+
	move.b  track,(a0)+
	clr.b	(a0)+		* side
        move.b  #1,(a0)+	* sector
	move.b	d2,(a0)+	* density byte
        move.b  (a1)+,(a0)+	* eot = spt
	move.b	(a1)+,(a0)+	* gpl
	move.b	(a1)+,(a0)+	* dtl
	rts

setdma:
* set dma address for data transfer
	move.l	#buffer,dma	* dma transfer is to or from buffer
        lea   dma+1,a0
        move.b  (a0)+,ddma
        move.b  (a0)+,ddma
        move.b  (a0)+,ddma
	rts

command:
	bsr	setdma
* send 9 bytes from command buffer to disk controller
        lea   cmd,a0		* point a0 at command buffer
        moveq   #8,d3		* send 9 bytes
com:
        move.b  (a0)+,d4
        bsr     send
        dbra    d3,com
	bsr	waitint		* wait until done
        bsr	results		* read all result bytes
        rts

read:
* routine to read a physical track
	bsr	setcmd		* set up the command buffer
        moveq   #retries,d5
read1:
        bsr     command		* issue command
	move.w	stat,d4		* was disk i/o successful?
	and	#$f8ff,d4	* ignore drive number
	sub	#$4080,d4	* 4080 is status for good read or write
	beq	readx		* exit if ok
	move.w	d4,d6		* otherwise:
	and.b	#8,d4		* drive not ready?
	beq	readtry
	move.l	#nrerr,d1	* display message
	bsr	derror
readtry:
        dbra    d5,read1	* retry
readx:
        rts

setbuffer:
	bsr	setdma		* use buffer
	lea	buffer,a0
	moveq	#1,d2		* sector nuumber
	clr.l	d3		* loop index
	move.b	density,d3
	mulu	#4,d3
	add.l	#gaps,d3
	move.l	d3,a2
	move.b	(a2),d3	* sectors per track
	sub	#1,d3
setbuf1:
	move.b	track,(a0)+
	move.b	side,d0
	and	#1,d0
	move.b	d0,(a0)+
	move.b	d2,(a0)+	* sector
	move.b	density,(a0)+
	add	#1,d2
	dbra	d3,setbuf1
	rts

form1:
* try once to format one side of one cylinder
	move.b	density,d5
	bsr	setbuffer
	move.b	#$0d,d4
	tst.b	d5
	beq	for1
	or.b	#$40,d4
for1:
	bsr	send		* command
	move.b	side,d4
	and	#4,d4
	or.b	drive,d4
	bsr	send		* drive and side
	move.b	d5,d4
	bsr	send		* density
	move.b	(a2),d4
	bsr	send		* spt
	move.b	3(a2),d4
	bsr	send		* gap3
	move.b	#$e5,d4
	bsr	send		* fill character
	bsr	waitint
	bsr	results
	rts

format1:
* format one side of one cylinder with retries
	move	#7,d6
forma1:
	bsr	form1
	lea	stat,a0
	move.b	(a0)+,d0	* get st0
	and.b	#$f8,d0		* ignore drive number
	or.b	(a0)+,d0	* or st1
	or.b	(a0)+,d0	* or st2
	dbeq	d6,forma1
	rts


	.data

dma:	.dc.l	buffer
drive:	.dc.b	0
track:	.dc.b	0
side:	.dc.b	0
sides:	.dc.b	0
drvstat:
	.dc.b	0
density:
	.dc.b	0
dense:	.dc.b	0
signon:
	.dc.b	13,10,'Floppy Disk Formatter 1.0',13,10
	.dc.b	'Copyright (c) 1983 CompuPro',13,10,'$'
qdrive:
	.dc.b	13,10,'Which drive? (A-D): $'
qdense:
	.dc.b	13,10,'Which density? (0-3): $'
form:
	.dc.b	13,10,'Formatting:',13,10,'$'
certain:
	.dc.b	13,10,'Type space to proceed. $'
qagain:
	.dc.b	13,10,'Do you want to format again? $'
nrerr:	.dc.b	' Not ready','$'
wperr:	.dc.b	' Write protected','$'
drverr:	.dc.b	' on drive ','$'
qmess:	.dc.b	' Type R to retry, A to abort ','$'

gaps:	.dc.b	26,$07,$80,$1b
	.dc.b	26,$0e,$ff,$36
	.dc.b	15,$1b,$ff,$54
	.dc.b	08,$35,$ff,$74

	.bss

cmd:    .ds.b   10
stat:   .ds.b   8

buffer:	.ds.b	$2000

	.end
