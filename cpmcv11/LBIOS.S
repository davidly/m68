*************************************************************************
*									*
*	CP/M-68K Loader BIOS						*
*	for CompuPro CPU-68K and Disk 1 floppy disk controller		*
*	by Michael A. Perry						*
*	Double Density							*
*	with full track buffering					*
*									*
*	last modified 10/14/83 by J. R. Stoner				*
*									*
*************************************************************************

syssup	=	1		* zero to select the system support console

iobase	=	$ff0000		* base of 68000's memory mapped i/o
sio	=	iobase+$10	* serial i/o data port
siostat	=	sio+1		* serial i/o status port
select	=	sio+7		* Interfacer III or IV user number
console	=	7		* Interfacer III or IV console user number
printer	=	4		* Interfacer III or IV printer user number
prnter1	=	5		* Interfacer III or IV UL1 user number

sysio	=	iobase+$5c	* base port for the system support
sysst	=	sysio+1		* status port for same

dstat	=	iobase+$c0	* disk status port
ddata	=	dstat+1		* disk data port
ddma	=	dstat+2		* disk dma port

dpblen	=	16
retries	=	7


	.globl	_bios

init:
	nop

	.ifeq	syssup

	move.b	#$ee,sysio+2	* set system support to 9600, 8bit, none
	move.b	#$3e,sysio+2
	move.b	#$27,sysio+3

	.endc
	.ifne	syssup

	move.b	#console,select	* initialize the console port
	move.b	#$ee,sio+2	* to 9600 baud, 8 bit, no parity
	move.b	#$3e,sio+2
	move.b	#$27,sio+3

	.endc

	move.b	#printer,select	* initialize the LPT port
	move.b	#$ee,sio+2	* to 9600 baud, 8 bit, no parity
	move.b	#$7e,sio+2
	move.b	#$27,sio+3
	move.b	#prnter1,select	* initialize the UL1 port
	move.b	#$ee,sio+2	* to 9600 baud, 8 bit, no parity
	move.b	#$7e,sio+2
	move.b	#$27,sio+3
	bsr	specify		* set disk stepping rate
	rts

_bios:
	cmpi	#functions,d0
	bge	nogood
	lsl	#2,d0		* multiply bios function by 4
	movea.l	6(pc,d0),a0	* get handler address
	jsr	(a0)		* call handler
nogood:
	rts

table:
	.dc.l  init
	.dc.l  nogood
	.dc.l  nogood
	.dc.l  nogood
	.dc.l  conout
	.dc.l  nogood
	.dc.l  nogood
	.dc.l  nogood
	.dc.l  nogood
	.dc.l  seldsk
	.dc.l  settrk
	.dc.l  setsec
	.dc.l  setdma
	.dc.l  read
	.dc.l  nogood
	.dc.l  nogood
	.dc.l  sectran
	.dc.l  nogood
	.dc.l  getseg
	.dc.l  nogood
	.dc.l  nogood
	.dc.l  nogood
	.dc.l  setexc

	functions=(*-table)/4

	.ifne	syssup

	nop			* because of the assembler bug
conout:
	move.b	#console,select
emit:
	move.b	siostat,d0	* get status
	and.b	#$1,d0		* wait for transmitter buffer empty
	beq	emit
	move.b	d1,sio		* and output it
	rts

	.endc
	.ifeq	syssup

	nop			* because of the assembler bug
conout:
	move.b	sysst,d0
	and.b	#$1,d0
	beq	conout
	move.b	d1,sysio
	rts

	.endc

seldsk:	
*	select disk A
*	return its dph in d0.l
	move.l	#dph0,d0	* point d0 at correct dph
* find the density of the disk, and set
* its density flag, dpb and xlt to the new values
	move.l	d0,a3		* save dph address
	bsr	restore		* home new disk
	move.b	#1,d6
	bsr	seek		* seek track one (zero is single dens.)
	bsr	rid		* get density
	moveq	#0,d7
	move.l	d7,d3
	move	stat,d2
	bne	setd0		* bad status means single density, else
	move.b	stat+6,d3	* get density byte
setd0:
	move.b	d3,density	* update density
	bsr	sds		* get sides
	move.b	d5,sides	* update side flag
	move.l	d3,d4		* d3 and d4 have density (0..3)
	mulu	#dpblen*2,d3
	tst.b	d5
	beq	setd1
	add	#dpblen,d3	* double sided
setd1:
	add.l	#dpb0,d3	* point d3 at selected dpb
	mulu	#4,d4
	add.l	#xlts,d4
	move.l	d4,a1	* point a1 into table of translation tables
	move.l	(a1),(a3)	* update xlt
	move.l	d3,14(a3)	* update dpb
	move.l	a3,d0		* restore dph to d0
	rts

settrk:
* set track number to value in d1
	move.w	d1,cpmtrk
	rts

setsec:
* set logical sector number to value in d1
	move.b	d1,cpmsec
	rts

sectran:
* translate sector in d1 using translation table pointed to by d2
* return result in d0
	move.l	d2,a0
	ext.l	d1
	move.b	#0(a0,d1),d0
	ext.l	d0
	rts

setdma:
* set data address to value in d1
	move.l	d1,cpmdma
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

specify:
* set disk stepping rate and head load time
        moveq   #3,d4
        bsr     send

	.ifeq	syssup

        moveq   #$df,d4		* 3ms/step

	.endc
	.ifne	syssup

	moveq	#$8f,d4

	.endc

        bsr     send
        moveq   #$46,d4		* 70ms head load
        bsr     send
        rts

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
        cmpi.b  #$20,d4		* successful seek?
        rts

restore:
* home the disk
        moveq   #2,d2		* 3 retries
rest1:
        moveq   #$7,d4
        bsr     send
        move.b  #0,d4
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
        move.b  #0,d4
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
	move.b	#0,d4
	bsr	send
	bsr	results
	move.b	stat,d5
	and.b	#8,d5		* select double sided bit
	rts

rid:
* Read ID field: find density
	move.b	#$4a,d4
	bsr	send
	move.b	#0,d4
	bsr	send
	bsr	waitint
	bsr	results		* read all result bytes
	rts

setcmd:
* set up the command buffer for the next read or write
	moveq	#0,d2
	move	track,d6
	beq	setcmd1		* force single density on track 0
	move.b	density,d2	* get density byte
	beq	setcmd1		* if not single density
	or.b	#$40,cmd	* then set mfm bit
setcmd1:
	move.l	d2,d3		* d2 and d3 have density byte = n
	mulu	#3,d3		* index into gap table
	add.l	#gaps,d3
	move.l	d3,a1		* point a1 at gpl/dtl entry
	lea	cmd+1,a0	* set up the command buffer
        move.b  side,d5
	lsl.b	#2,d5		* side is bit 2 of drive byte
	move.b	d5,(a0)+
        move.b  track+1,(a0)+	* low byte of track number
	move.b	side,(a0)+	* redundant side byte
        move.b  #1,(a0)+	* sector
	move.b	d2,(a0)+	* density byte
        move.b  (a1)+,(a0)+	* eot = spt
	move.b	(a1)+,(a0)+	* gpl
	move.b	(a1)+,(a0)+	* dtl
	rts

command:
* set dma address for data transfer
        lea   dma+1,a0
        move.b  (a0)+,ddma
        move.b  (a0)+,ddma
        move.b  (a0)+,ddma
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

rdwr:
* Common routine to read or write a physical track
	move.l	#buffer,dma	* dma transfers are to and from buffer
	move.w	track,d6	* perform a seek to desired track
	bsr	seek
	bsr	setcmd		* set up the command buffer
        moveq   #retries,d5
rdwr1:
        bsr     command		* issue command
	move.w	stat,d4		* was disk i/o successful?
	and	#$f8ff,d4	* ignore the minor device
	sub	#$4080,d4	* 4080 is status for good read or write
	beq	rdwrok		* exit if ok
        dbra    d5,rdwr1	* retry
rdwrerr:
	moveq	#1,d0		* return error status
	rts
rdwrok:
        moveq	#0,d0		* return good status
        rts

read:
* Read one sector from requested disk, track, sector to dma address
* Retry if necessary; return zero in d0 if ok.
* make certain that the desired data is in the buffer
	moveq	#0,d0
	move.l	d0,d6
	move.l	d0,d5
	move	cpmtrk,d6	* get logical track
	clr.b	side		* default to side zero
	tst.b	sides		* if double sided:
	beq	inbuf3
	move	d6,d5
	and	#1,d5		* get side number
	move.b	d5,side
	lsr	#1,d6		* get track number
inbuf3:
	move	d6,track
	cmp.b	bufsid,d5
	bne	inbuf1
	cmp	buftrk,d6
	beq	inbuf2		* new track, perform read
inbuf1:
	move.b	#6,cmd		* select read
	bsr	rdwr		* perform read
	bne	readx		* exit on error
	move	track,buftrk	* update buffer track number
	move.b	side,bufsid
inbuf2:
	moveq	#0,d2
	move.b	cpmsec,d2
	sub	#1,d2		* number from 0
	mulu	#128,d2		* d2 has offset into buffer
	add.l	#buffer,d2	* d2 has data address
	move.l	cpmdma,a1	* a1 has dma address
	move.l	d2,a0		* a0 points to data
	moveq	#127,d2
mov:	move.b	(a0)+,(a1)+
	dbra	d2,mov
readx:
	rts

getseg:
	move.l	#region,d0	* return address of memory region table
	rts

setexc:
	andi.l	#$ff,d1		* do only for exceptions 0 - 255
	lsl	#2,d1		* multiply exception nmbr by 4
	movea.l	d1,a0
	move.l	(a0),d0		* return old vector value
	move.l	d2,(a0)		* insert new vector
	rts


	.data

region:	.dc.w	1	* one region
	.dc.l	$2000	* start of TPA
	.dc.l	$10000	* length

*	disk parameter headers
	.even

dph0:	.dc.l	xlt0
	.dc.w	0	* dummy
	.dc.w	0
	.dc.w	0
	.dc.l	dirbuf	* ptr to directory buffer
	.dc.l	dpb0	* ptr to disk parameter block
	.dc.l	0	* ptr to check vector
	.dc.l	0	* ptr to allocation vector

* disk parameter block

dpb0:	.dc.w	26	* sectors per track
	.dc.b	3	* block shift
	.dc.b	7	* block mask
	.dc.b	0	* extent mask
	.dc.b	0	* dummy fill
	.dc.w	242	* disk size
	.dc.w	63	* directory entries
	.dc.w	$c000	* directory mask
	.dc.w	16	* directory check size
	.dc.w	2	* track offset

dpb0d:	.dc.w	26	* sectors per track
	.dc.b	4	* block shift
	.dc.b	15	* block mask
	.dc.b	1	* extent mask
	.dc.b	0	* dummy fill
	.dc.w	242	* disk size
	.dc.w	127	* directory entries
	.dc.w	$c000	* directory mask
	.dc.w	32	* directory check size
	.dc.w	4	* track offset

dpb1:	.dc.w	52	* sectors per track
	.dc.b	4	* block shift
	.dc.b	15	* block mask
	.dc.b	0	* extent mask
	.dc.b	0	* dummy fill
	.dc.w	242	* disk size
	.dc.w	127	* directory entries
	.dc.w	$c000	* directory mask
	.dc.w	32	* directory check size
	.dc.w	2	* track offset

dpb1d:	.dc.w	52	* sectors per track
	.dc.b	4	* block shift
	.dc.b	15	* block mask
	.dc.b	0	* extent mask
	.dc.b	0	* dummy fill
	.dc.w	485	* disk size
	.dc.w	255	* directory entries
	.dc.w	$f000	* directory mask
	.dc.w	64	* directory check size
	.dc.w	4	* track offset

dpb2:	.dc.w	60	* sectors per track
	.dc.b	4	* block shift
	.dc.b	15	* block mask
	.dc.b	0	* extent mask
	.dc.b	0	* dummy fill
	.dc.w	261	* disk size
	.dc.w	127	* directory entries
	.dc.w	$c000	* directory mask
	.dc.w	32	* directory check size
	.dc.w	2	* track offset

dpb2d:	.dc.w	60	* sectors per track
	.dc.b	4	* block shift
	.dc.b	15	* block mask
	.dc.b	0	* extent mask
	.dc.b	0	* dummy fill
	.dc.w	523	* disk size
	.dc.w	255	* directory entries
	.dc.w	$f000	* directory mask
	.dc.w	64	* directory check size
	.dc.w	4	* track offset

dpb3:	.dc.w	64	* sectors per track
	.dc.b	4	* block shift
	.dc.b	15	* block mask
	.dc.b	0	* extent mask
	.dc.b	0	* dummy fill
	.dc.w	299	* disk size
	.dc.w	127	* directory entries
	.dc.w	$c000	* directory mask
	.dc.w	32	* directory check size
	.dc.w	2	* track offset

dpb3d:	.dc.w	64	* sectors per track
	.dc.b	4	* block shift
	.dc.b	15	* block mask
	.dc.b	0	* extent mask
	.dc.b	0	* dummy fill
	.dc.w	599	* disk size
	.dc.w	255	* directory entries
	.dc.w	$f000	* directory mask
	.dc.w	64	* directory check size
	.dc.w	4	* track offset

* sector translation tables

xlt0:	.dc.b	1,7,13,19,25
	.dc.b	5,11,17,23
	.dc.b	3,9,15,21
	.dc.b	2,8,14,20,26
	.dc.b	6,12,18,24
	.dc.b	4,10,16,22

xlt1:	.dc.b	1,2,19,20,37,38
	.dc.b	3,4,21,22,39,40
	.dc.b	5,6,23,24,41,42
	.dc.b	7,8,25,26,43,44
	.dc.b	9,10,27,28,45,46
	.dc.b	11,12,29,30,47,48
	.dc.b	13,14,31,32,49,50
	.dc.b	15,16,33,34,51,52
	.dc.b	17,18,35,36

xlt2:	.dc.b	1,2,3,4,17,18,19,20,33,34,35,36,49,50,51,52
	.dc.b	5,6,7,8,21,22,23,24,37,38,39,40,53,54,55,56
	.dc.b	9,10,11,12,25,26,27,28,41,42,43,44,57,58,59,60
	.dc.b	13,14,15,16,29,30,31,32,45,46,47,48

xlt3:	.dc.b	1,2,3,4,5,6,7,8
	.dc.b	25,26,27,28,29,30,31,32
	.dc.b	49,50,51,52,53,54,55,56
	.dc.b	9,10,11,12,13,14,15,16
	.dc.b	33,34,35,36,37,38,39,40
	.dc.b	57,58,59,60,61,62,63,64
	.dc.b	17,18,19,20,21,22,23,24
	.dc.b	41,42,43,44,45,46,47,48

xlts:	.dc.l	xlt0,xlt1,xlt2,xlt3
gaps:	.dc.b	26,$07,$80
	.dc.b	26,$0e,$ff
	.dc.b	15,$1b,$ff
	.dc.b	08,$35,$ff

	.bss

cmd:    .ds.b   10
stat:   .ds.b   8
cpmdma:	.ds.l	1
dma:	.ds.l	1
cpmsec:	.ds.b	1
sector:	.ds.b	1
cpmtrk:	.ds.w	1
buftrk:	.ds.w	1
track:	.ds.w	1
bufsid	.ds.b	1
side:	.ds.b	1
sides:	.ds.b	1
density:
	.ds.b	1		* current density

	.even
dirbuf:	.ds.b	128
buffer:	.ds.b	$2000

	.end
