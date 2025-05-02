*****************************************************************
*	 							*
* 	CP/M-68K BIOS						*
* 	for CompuPro CPU-68K and Disk 1 floppy disk controller	*
* 	by Michael A. Perry					*
* 	Double Density, Double Sided				*
* 	with track buffering					*
* 	with M-Drive/H support					*
* 	with DISK 2 support					*
*	with DISK 3 support					*
* 								*
* 	Last modified:	10/24/83 by J. R. Stoner		*
* 								*
*****************************************************************

	.globl	_init		* bios initialization entry point
	.globl	_ccp		* ccp entry point

model	=	2		* CP/M-68K version
make	=	1		* CP/M-68K correction number
patch	=	8		* CompuPro CP/M-68k CBIOS revision level

syssup	=	0		* zero for system support, 1 for I/O 3 or 4
lomem	=	$400		* bottom of tpa
himem	=	$35000		* top of TPA

xd8	=	0		* zero to select the second floppy disk drives
fdbase	=	0		* set to the first floppy major device number

hmdrive	=	0		* zero to select the M-DRIVE/H memory disk

d2m10	=	1		* zero to select 10 Mb hard disk (8")
d2m20	=	1		* zero to select 20 Mb hard disk (8")
d2f20b	=	1		* zero to select BE 20 Mb hard disk
d2f40b	=	1		* zero to select BE 40 Mb hard disk
d2base	=	4		* set to the first DISK2 major device number

d3m5	=	1		* zero to select  5 Mb hard disk (ST506)
d3m40	=	1		* zero to select 40 Mb hard disk (Q540)
d3base	=	4		* set to the first DISK3 major device number

stepr8	=	3		* seek step rate (ms)

uload8	=	240		* head unload time (ms)
hdlt8	=	35		* head settle time (ms)

srt8	=	16-stepr8
hut8	=	uload8/16

disk2	=	d2m10*d2m20*d2f20b*d2f40b
disk3	=	d3m5*d3m40
hard	=	disk2*disk3

d3scnt	=	9		* 9 1Kb sectors/track
d2scnt	=	11		* 11 1Kb sectors/track

d2_rhd	=	$e0		* DISK2 read header
d2_wrt	=	$d0		* DISK2 write data
d2_read	=	$c8		* DISK2 read data
d2_sin	=	$a0		* DISK2 step in
d2_sec	=	$98		* DISK2 set sector
d2_head	=	$90		* DISK2 set head
d2_cyl	=	$88		* DISK2 set cylinder
d2_strb	=	$80		* DISK2 drive strobe
d2_sou	=	$80		* DISK2 step out
d2_rst	=	$04		* DISK2 reset drive fault clear bit

d2_attn	=	$80		* DISK2 interrupt acknowledge status bit
d2_tout	=	$40		* timeout error bit
d2_crc	=	$20		* CRC error bit
d2_ovr	=	$10		* data transfer overrun bit
d2_nrdy	=	$08		* drive not ready status bit
d2_sekd	=	$04		* seek complete flag
d2_wrtf	=	$02		* write fault (0 = true)
d2_cyl0	=	$01		* cylinder 00 (0 = true)

selpri	=	10		* the DMA priority of the selector channel
selbyte	=	$2f-selpri	* selector channel command byte

d3_attn	=	0		* the command byte for waking up the DISK3
d3_rst	=	1		* the command byte for resetting the DISK3

d3_nop	=	0		* iopb command for a NOP
d3_glob	=	2		* iopb command for writing global variables
d3_spec	=	3		* iopb command for a specify
d3_map	=	4		* iopb command for a set_map
d3_home	=	5		* iopb command for a recalibrate
d3_rdwr	=	8		* iopb command for read or write data
d3_stat	=	12		* iopb command for reading the drive status

d3_cmpl	=	$ff		* iopb return value for success

d3bmtrk	=	2		* relocated data head
d3bmcyl	=	0		* relocation cylinder
d3bmlen	=	d3scnt-1
d3bmsec	=	d3bmlen*8	* logical sector of the sector map

defiopb	=	$50		* the address of the first iopb at reset

k_cr	=	13		* ASCII newline
k_lf	=	10		* ASCII line feed
k_cls	=	26		* clear screen for VT52 emulations (Lear,TVI)

dpblen	=	16		* length of a DPB

* here be the I/O address standard assignments:

iobase	=	$ff0000		* base of 68000's memory mapped i/o

sysdata	=	iobase+$5c	* System Support console port
sysstat	=	sysdata+1

sio	=	iobase+$10	* serial i/o data port
siostat	=	sio+1		* serial i/o status port
select	=	sio+7		* Interfacer III or IV user number

mddata	=	iobase+$c6	* memory disk data port
mdadr	=	mddata+1	* memory disk address port

dstat	=	iobase+$c0	* disk status port
ddata	=	dstat+1		* disk data port
ddma	=	dstat+2		* disk dma port

d2ctl	=	iobase+$c8	* DISK2 control port
d2stat	=	d2ctl		* DISK2 status port
d2data	=	d2ctl+1		* DISK2 data port
selchan	=	iobase+$f0	* SELECTOR CHANNEL port

d3port	=	iobase+$90	* DISK3 port

* relative user numbers on an INTERFACER 3/INTERFACER 4 board
console	=	7
printer	=	4
prnter1	=	5

* disk types in the software DPB extension:

* 8" floppy disks

fp8s1d0	=	$00		* 1 sided, density 0: 128b
fp8s2d0	=	$01		* 2 sided, density 0: 128b
fp8s1d1	=	$02		* 1 sided, density 1: 256b
fp8s2d1	=	$03		* 2 sided, density 1: 256b
fp8s1d2	=	$04		* 1 sided, density 2: 512b
fp8s2d2	=	$05		* 2 sided, density 2: 512b
fp8s1d3	=	$06		* 1 sided, density 3: 1024b
fp8s2d3	=	$07		* 2 sided, density 3: 1024b

maxftyp	=	fp8s2d3+1	* number of 8" floppy disk DPB's

dsk2typ	=	$40		* DISK2 DPB type
dsk3typ	=	$50		* DISK3 DPB type
memtype	=	$80		* M-DRIVE/H DPB type

dphlen	=	26		* length of disk parameter header

retries	=	10		* number of retries for floppy I/O

_init:
	move.l	#signon,a0	* say hello
	bsr	type
	clr.b	dirty

	.ifeq	disk2

	bsr	d2init

	.endc
	.ifeq	disk3

	clr.b	d3iopb+2	* right now we initialize only drive zero
	bsr	d3spec		* go specify and set up the mapping
	tst.b	d0
	bne	inita		* will be zero here if no problems
	lea	d3iopb+3,a2	* point at some of the iopb
	clr.b	(a2)+		* set up in track/sector mode
	move.b	#retries,(a2)+	* set up for a retry count of 10
	move.b	#4,(a2)+	* set up for 4 drives in the system
	move.l	#d3_glob,d0	* set up the global parameter command
	bsr	d3exec		* and go do it
inita:
	nop			* yuck!

	.endc
	.ifeq	hmdrive

	bsr	setsize		* auto-size M-Drive/H

	.endc

	move.l	#trapentry,$8c	* set up trap #3 handler
	clr.l	d0		* log on disk A, user 0
	rts

trapentry:
	cmpi	#functs,d0	* function number out of bounds
	bge	badtrap		* yes
	lsl	#2,d0		* multiply bios function by 4
	movea.l	6(pc,d0),a0	* get handler address
	jsr	(a0)		* call handler
badtrap:
	rte

table:
	.dc.l  _init		* cold boot
	.dc.l  wboot		* warm boot
	.dc.l  constat		* console status
	.dc.l  conin		* console input
	.dc.l  conout		* console output
	.dc.l  lstout		* printer output
	.dc.l  pun		* user device output
	.dc.l  rdr		* user device input
	.dc.l  home		* home the logical disk
	.dc.l  seldsk		* select a logical disk
	.dc.l  settrk		* set a logical track
	.dc.l  setsec		* set a logical sector
	.dc.l  setdma		* set the DMA pointer
	.dc.l  read		* read a logical sector
	.dc.l  write		* write a logical sector
	.dc.l  listst		* printer status
	.dc.l  sectran		* translate logical to physical sectors
	.dc.l  setdma		* for historical reasons
	.dc.l  getseg		* get the memory segment
	.dc.l  getiob		* get the I/O map byte
	.dc.l  setiob		* set the I/O map byte
	.dc.l  flush		* flush the disk buffers
	.dc.l  setexc		* set a trap vector

	functs=(*-table)/4

* discard buffer
abort:
	clr.b	cpmflg		* default to the 8" SSSD disk
	clr.b	dirty		* do not flush the floppy disks
wboot:
	clr.l	news		* mark all drives as new
	bsr	flush		* write buffer to disk
	moveq	#$ff,d3		* mark buffer as empty
	move.b	d3,bufdrv	* no buffered drive
	move	d3,buftrk	* no buffered track
	move.b	d3,bufsid	* no buffered head
	jmp	_ccp		* go to the CP/M system now

	.ifeq	syssup		* system support console being used here

constat:
	move.b	sysstat,d0	* get status byte
	andi.w	#2,d0		* data available bit on?
	bne	yes		* branch if true
	moveq	#0,d0		* set result to false
	rts
yes:
	moveq.l	#$ff,d0		* set result to true
	rts

conin:
	move.b	sysstat,d0	* get status byte
	andi.w	#2,d0		* data available bit on?
	beq	conin		* wait until key pressed
	move.b	sysdata,d0	* get key
	and.l	#$7f,d0		* clear all but low 7 bits
	rts

conout:
	move	d1,-(a7)	* save character
	bsr	flush		* write data to disk
	move	(a7)+,d1	* restore character
emit:
	move.b	sysstat,d0	* get status
	and.b	#$1,d0		* wait for transmitter buffer empty
	beq	emit
	move.b	d1,sysdata	* and output it
	rts

	.endc
	.ifne	syssup		* interfacer console being used here

constat:
	move.b	#console,select
	move.b	siostat,d0	* get status byte
	andi.w	#2,d0		* data available bit on?
	bne	yes		* branch if true
	moveq	#0,d0		* set result to false
	rts
yes:
	moveq.l	#$ff,d0		* set result to true
	rts

conin:
	move.b	#console,select
	move.b	siostat,d0	* get status byte
	andi.w	#2,d0		* data available bit on?
	beq	conin		* wait until key pressed
	move.b	sio,d0		* get key
	and.l	#$7f,d0		* clear all but low 7 bits
	rts

conout:
	move	d1,-(a7)	* save character
	bsr	flush		* write data to disk
	move	(a7)+,d1	* restore character
emit:
	move.b	#console,select
	move.b	siostat,d0	* get status
	and.b	#$1,d0		* wait for transmitter buffer empty
	beq	emit
	move.b	d1,sio		* and output it
	rts

	.endc

listst:
	move.b	cpmiob,d0
	and.l	#$c0,d0
	beq	lst2
	move.b	#prnter1,select	* select the relative user for the UL1 device
	bra	lst1
lst2:
	move.b	#printer,select	* select the relative user for the LPT device
lst1:
	move.b	siostat,d0	* get the status
	and.b	#1,d0		* mask the bit
	bne	yes		* if a character is ready
	moveq	#0,d0		* set for not ready
	rts

pun:
lstout:
	move.b	#printer,select	* select the printer or user devce
	move.b	siostat,d0	* get status
	and.b	#$1,d0		* wait for transmitter buffer empty
	beq	lstout
	move.b	d1,sio		* and output it
	rts

rdr:
	move.b	#printer,select	* select the user device
	move.b	siostat,d0	* get status byte
	andi.w	#2,d0		* data available bit on?
	beq	rdr		* wait until character available
	move.b	sio,d0		* get character
	rts

cr:
	move.b	#k_cr,d1	* emit a newline
	bsr	emit
	move.b	#k_lf,d1	* emit a linefeed too
	bsr	emit
	rts

type:
	move.b	(a0)+,d1	* get the next character
	beq	typex		* if zero we end the string
	bsr	emit		* else we emit the character
	bra	type		* go back for more
typex:
	rts

derror:
	bsr	type		* type the selected string
	move.l	#drverr,a0	* print the prompt
	bsr	type
	move.b	drive,d1	* get the logical drive (major device)
	add	#'A',d1		* bias for ASCII
	bsr	emit		* type it
	bsr	cr		* newline
	move.l	#qmess,a0	* ask for something to do
	bsr	type
	bsr	conin		* get a character
	or.b	#' ',d0		* bias to make it lower case
	cmp.b	#'r',d0		* do we have to retry it
	bne	abort		* no so we abort the I/O
	bsr	cr		* newline
	rts

*	select disk given by register d1.b
*	return its dph in d0.l
seldsk:
	moveq	#0,d0		* default to a bad type
	cmp.b	#16,d1		* major device out of bounds?
	bcc	selx		* if yes
	move.b	d1,cpmdrv	* set the current major device
	mulu	#4,d1		* set up for a pointer
	lea	drvtab,a0	* point a0 at the DPH address table
	add.l	d1,a0		* index it
	move.l	(a0),d0		* get the DPH address into d0
	beq	selx		* if zero it is a bad drive
	move.l	d0,a0		* point a0 at the DPH
	move.l	14(a0),d1	* point d1 at the DPB
	sub.l	#dpb0,d1	* we are going to get the offset from dpb0
	divu	#dpblen,d1	* d1 contains the DPB number
	lea	typbase,a1	* point a1 at the DPB extension table
	move.b	0(a1,d1),cpmflg	* set the major device type current
	move.b	cpmflg,d1	* get the major device type
	cmp.b	#maxftyp,d1	* is it a floppy disk?
	bcs	selfloppy	* if yes

	.ifeq	hmdrive

	cmp.b	#memtype,d1	* is it a memory drive?
	bne	selo		* if not, we already have a DPH in d0
	move.l	#dphm,d0	* point d0 at the memory drive DPH
	bra	selx		* and leave
selo:
	nop

	.endc
	.ifeq	disk2

	cmp.b	#dsk2typ,d1	* is it a DISK2 hard disk?
	beq	selx		* if yes, the DPH is already in d0

	.endc
	.ifeq	disk3

	cmp.b	#dsk3typ,d1	* is it a DISK3 hard disk?
	beq	selx		* if yes, the DPH is already in d0

	.endc

	clr	d0		* if not, return a bad major device
	bra	selx
selfloppy:
	lea	news,a5		* point a5 at the disk mount table
	move.b	cpmdrv,d1	* get the major device number (0..3)
	tst.b	0(a5,d1)	* use it to check the current mount status
	bne	selx		* if the drive is already mounted
	bsr	setdense	* go get the density and mark it as mounted
selx:
	rts

* set track number to 0
home:
	clr	d1		* we defer any seeking for later

* set track number to value in d1
settrk:
	move.w	d1,cpmtrk	* make the logical track as current
	rts			* defer the head seeking for later

* set logical sector number to value in d1
setsec:
	move.b	d1,cpmsec	* make the logical sector as current
	rts

* use table in d2 to translate logical sector in d1 to the physical sector
sectran:
	tst.l	d2		* is the table address zero
	beq	notran		* if yes
	move.l	d2,a0		* point a0 at the table
	ext.l	d1		* make d1 a longword
	move.b	#0(a0,d1),d0	* index d1 into a0 for the skewed sector
	ext.l	d0		* make d0 a longword
	bra	sectranx	* and leave
notran:
	move	d1,d0		* use the logical sector as the return value
sectranx:
	rts

* set data address to value in d1
setdma:
	move.l	d1,cpmdma	* make the new address current
	rts

read:
	move.b	cpmflg,d0	* get the major device type

	.ifeq	hmdrive

	cmp.b	#memtype,d0	* memory drive?
	beq	vread		* perform virtual disk read

	.endc

        bsr     inbuffer	* read sector into buffer
	tst	d0
	bne	readx		* exit on error
	move.l	d2,a0		* a0 points to data
	move.l	d1,a1		* a1 points to dma address
	bsr	transfer	* move data from buffer to dma address
readx:
	rts

write:
	move.b	d1,cpmtyp	* save the write type status for later

	.ifeq	hmdrive

	move.b	cpmflg,d0	* get the major device type
	cmp.b	#memtype,d0	* memory drive?
	beq	vwrite		* perform virtual disk write

	.endc

	bsr	inbuffer	* read track into buffer
	tst	d0		* if the pre-read was faulty
	bne	writex		* exit on error
	move.l	d1,a0		* a0 points to new data
	move.l	d2,a1		* a1 points into buffer
	bsr	transfer	* move data from dma address into buffer
	move.b	#1,dirty	* set the DISK1 dirty flag (default)
	move.b	cpmtyp,d1	* get the write type back
	cmp.b	#1,d1		* was the write to a directory sector?
	bne	writex		* if not
	bsr	flush		* if yes, go flush it now and not later
writex:
	rts

* write buffered data to disk, if it has been updated
flush:
	nop

	.ifeq	hard

	move.b	bufdrv,d0	* get the currently buffered major device
	and.l	#$f,d0		* mask it
	lea	hdtab,a5	* get the drive select table base
	tst.b	0(a5,d0)	* see if the currently buffered drive is hard
	bne	d2flush		* if non-zero we do a hard disk flush

	.endc

	clr.l	d0		* default is return successful
	tst.b	dirty		* has data been written into buffer?
	beq	flushx		* exit if not
	move.b	bufdrv,d7	* get the current major device
	cmp.b	#4,d7		* is it out of bounds (0..3)
	bpl	flushx		* if yes, exit now
	move.b	bufdrv,drive	* make the buffered major device current
	move	buftrk,track	* and the cylinder
	move.b	bufsid,side	* and the head
	move.b	#5,cmd		* set up the FDC write data command
	bsr	rdwr		* go do the I/O
flushx:
	clr.b	dirty		* always clear dirty flag
	rts

getseg:
	move.l	#region,d0	* return address of memory region table
	rts

* return the current I/O byte in d0
getiob:
	move.b	cpmiob,d0
	and.l	#$ff,d0
	rts

* set the current I/O byte to whatever is in d1
setiob:
	move.b	d1,cpmiob
	clr.l	d0
	rts

setexc:
	andi.l	#$ff,d1		* do only for exceptions 0 - 255
	lsl	#2,d1		* multiply exception nmbr by 4
	movea.l	d1,a0
	move.l	(a0),d0		* return old vector value
	move.l	d2,(a0)		* insert new vector
	rts

* send byte in d4 to disk controller
send:
        btst    #7,dstat	* is the FDC command buffer busy?
        beq     send		* if yes
        move.b  d4,ddata	* if not go send the command byte
        rts

* read all result bytes from disk controller
results:
        lea   stat,a0		* point a0 at the first byte of the table
res1:
        btst    #7,dstat	* is the FDC command buffer busy?
        beq     res1		* if yes
        btst    #6,dstat	* is the CPU expecting a status
        beq     resx		* if not
        move.b  ddata,(a0)+	* get the status byte into the next entry
        bra     res1		* go check it for any more
resx:   rts

* set disk stepping rate and head load time
specify:
        moveq   #3,d4		* send the specify command
        bsr     send
	moveq	#srt8*16+15,d4	* send the step rate and the head unload time
        bsr     send
        moveq   #hdlt8,d4	* send the head load time
        bsr     send
        rts

* wait until Disk 1 interrupt flag becomes true
waitint:
	btst	#7,ddma		* is the FDC still doing some dma?
	beq	waitint		* if yes
	rts

* Sense Interrupt Status: must be used after seek or home
sis:
	bsr	waitint		* see if the FDC is ready for something new
        moveq   #8,d4		* send the sense interrupt status command
        bsr     send
        bsr	results		* read all result bytes
        move.b  stat,d4		* status for correct drive?
	move.b	drive,d1	* get the FDC major device (0..3)
	eor	d4,d1		* XOR with the device number from the FDC
	and	#3,d1		* mask the lower two bits
	bne	sis		* should be zero - try again if not
        andi.b  #$f8,d4		* ignore drive number
        cmpi.b  #$20,d4		* successful seek?
	beq	sisx		* exit if ok
	move.b	stat,d4		* get the FDC status byte s0
	andi.b	#$18,d4		* is drive not ready?
	beq	sisx		* exit if ok
	move.l	#nrerr,a0	* else print message
	bsr	derror		* and ask to retry
sisx:	
        rts

* home the disk
restore:
        moveq   #2,d2		* 3 retries
rest1:
        moveq   #$7,d4		* send the recalibrate disk command
        bsr     send
        move.b  drive,d4	* send the current major device (0..3)
        bsr     send
        bsr     sis		* get status
        dbeq    d2,rest1	* retry on error
        rts

* position the disk head to the track in d6.b
seek:
        moveq   #2,d2		* 3 retries
seek1:
        moveq   #$0f,d4		* send the seek command
        bsr     send
        move.b  drive,d4	* send the current major device (0..3)
        bsr     send
        move.b  d6,d4		* send the new cylinder
        bsr     send
        bsr     sis		* get status
        dbeq    d2,seek1	* retry on error
	beq	seekx		* exit if ok
	bsr	restore		* home disk if the retries did not help
seekx:
        rts

* Sense Drive Status
* tests for double sided
sds:
	move.b	#4,d4		* send the sense drive status command
	bsr	send
	move.b	drive,d4	* send the current major device (0..3)
	bsr	send
	bsr	results		* get all of the status bytes
	move.b	stat,d5
	and.b	#8,d5		* mask only the double sided bit
	rts

* Read ID field: find density
rid:
	move.b	#$4a,d4		* send the read ID/MFM command
	bsr	send
	move.b	drive,d4	* send the current major device (0..3)
	bsr	send
	bsr	waitint		* wait until done
	bsr	results		* get all of the status bytes
	rts

* find the density of the disk in the selected drive, and set
* its density flag, dpb and xlt to the new values
setdense:
	move.l	d0,a3		* save the DPH address
	move.b	cpmdrv,drive	* make the new major device current
	bsr	restore		* recalibrate the new disk
	bsr	sds		* check the number of sides
	move.b	#1,d6		* set cylinder 1 (to see if the disk is MFM)
	bsr	seek		* move the head
	bsr	rid		* get density into the status table
	moveq	#0,d7		* set the table pointers to the default values
	move.l	d7,d3
	move	stat,d2		* get the FDC status byte s0
	and	#$fcff,d2	* ignore drive number
	bne	setd0		* bad status means single density, else
	move.b	stat+6,d3	* get density byte (0..3)
setd0:
	lea	dens,a0		* point to density table
	move.b	drive,d7	* index by minor device
	move.b	d3,0(a0,d7)	* update density table entry
	lea	sides,a0	* point to double sided flags
	move.b	d5,0(a0,d7)	* update side flag
	lea	news,a0		* point to new drive flags
	move.b	#$ff,0(a0,d7)	* mark drive as currently mounted
	move.l	d3,d4		* d3 and d4 have density (0..3)
	mulu	#dpblen*2,d3	* index the pointer by DPB density type (even)
	tst.b	d5		* d5 is 1 for two-sided disk
	beq	setd1		* if one-sided
	add	#dpblen,d3	* set the pointer to the next odd DPB number
setd1:
	add.l	#dpb0,d3	* point d3 at selected DPB (must be the lowest)
	mulu	#4,d4		* index the XLT pointer by the density
	add.l	#xlts,d4	* point d4 at the XLT table address list
	move.l	d4,a1		* point a1 at it too
	move.l	(a1),(a3)	* move the XLT address to the DPH XLT location
	move.l	d3,14(a3)	* move the DPB address to the DPH DPB location
	move.l	a3,d0		* return the DPH
	rts

* set up the command buffer for the next read or write
setcmd:
	moveq	#0,d7
	move.l	d7,d2
	move.b	drive,d7	* index by drive number
	move	track,d6
	beq	setcmd1		* force single density on track 0
	lea	dens,a1		* point a0 at density table
	move.b	0(a1,d7),d2	* get density byte
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
	or.b	d5,d7
	move.b	d7,(a0)+
	move.b	track+1,(a0)+	* low byte of track number
	move.b	side,(a0)+	* redundant side byte
        move.b  #1,(a0)+	* setor
	move.b	d2,(a0)+	* density byte
	move.b	(a1)+,(a0)+	* eot = spt
	move.b	(a1)+,(a0)+	* gpl
	move.b	(a1)+,(a0)+	* dtl
	rts

* set dma address for data transfer
command:
        lea	dma+1,a0	* point a0 at the current dma address
        move.b	(a0)+,ddma	* send the high byte
        move.b	(a0)+,ddma	* send the middle byte
	move.b	(a0)+,ddma	* send the low byte
        lea	cmd,a0		* point a0 at command buffer
	moveq	#8,d3		* set up for 8 bytes and fall through...

com:
        move.b  (a0)+,d4	* get the next byte
        bsr     send		* and send it
        dbra	d3,com		* maybe send some more
        bsr	waitint		* wait for the command to finish
	bsr	results		* read all result bytes
        rts

* Common routine to read or write a physical track
rdwr:
	move.l	#buffer,dma	* dma transfers are to and from buffer
	move.w	track,d6	* perform a seek to desired cylinder
	bsr	seek
	bsr	setcmd		* set up the command buffer
        moveq   #retries,d5	* 10 retries
rdwr1:
        bsr     command		* issue command
	move.w	stat,d4		* was disk i/o successful?
	and	#$f8ff,d4	* ignore the minor device
	sub	#$4080,d4	* 0x4080 is status for good read or write
	beq	rdwrok		* exit if ok
	move.w	d4,d6		* otherwise:
	and.b	#8,d4		* drive not ready?
	beq	rdwr2
	move.l	#nrerr,a0	* display message
	bsr	derror		* and ask for a retry or abort
	bra	rdwrtry		* go do a retry
rdwr2:
	and.b	#2,d6		* write protected?
	beq	rdwrtry		* if not, this is a weird error, go do a retry
	move.l	#wperr,a0	* display message for write protected
	bsr	derror		* and ask for a retry or abort
rdwrtry:
        dbra    d5,rdwr1	* retry
rdwrerr:
	moveq	#1,d0		* return the error condition
	rts
rdwrok:
        moveq	#0,d0		* return success
        rts

* make certain that the desired data is in the buffer
* if it was not, then read it from the disk after flushing old contents
inbuffer:
	nop

	.ifeq	hard

	move.b	cpmflg,d0	* get the major device

	.ifeq	disk2

	cmp.b	#dsk2typ,d0	* is it a DISK2?
	beq	d2buf		* if yes

	.endc
	.ifeq	disk3

	cmp.b	#dsk3typ,d0	* is it a DISK3
	beq	d3buf

	.endc
	.endc

	clr.l	d0		* clear the registers
	clr.l	d7
	clr.l	d6
	clr.l	d5
	clr.l	d4
	move.b	cpmdrv,d7	* get the selected major device
	move.b	d7,rdrive	* save it
	move	cpmtrk,d6	* get the selected logical track
	clr.b	rside		* default the saved side to zero
	lea	sides,a1	* point at the sidedness table
	tst.b	0(a1,d7)	* test the device for sidedness
	beq	inbuf3		* if not
	move	d6,d5		* copy the track
	and	#1,d5		* get head number
	move.b	d5,rside	* and save it
	lsr	#1,d6		* shift the physical cylinder by one (div2)
inbuf3:
	move	d6,rtrack	* save the physical cylinder
	lea	dens,a1		* point a1 at density table
	move.b	0(a1,d7),d2	* get density byte (0..3)
	cmp.b	bufdrv,d7	* compare the saved and current major device
	bne	inbuf1		* if not the same
	cmp	buftrk,d6	* compare the saved and current cylinders
	bne	inbuf1		* if not the same
	cmp.b	bufsid,d5	* compare the save and current heads
	beq	inbuf2		* if the same we do not do I/O yet
inbuf1:
	bsr	flush		* write old buffer contents
	move.b	rdrive,drive	* make the saved major device new
	move	rtrack,track	* make the saved cylinder new
	move.b	rside,side	* make the saved head new
	move.b	#6,cmd		* set up the command table for an FDC read data
	bsr	rdwr		* go do the I/O
	move.b	drive,bufdrv	* make the new major device buffered
	move	track,buftrk	* make the new cylinder current
	move.b	side,bufsid	* make the new head current
inbuf2:
	move.l	#0,d2		* clear the register
	move.b	cpmsec,d2	* get the selected logical sector
	sub	#1,d2		* offset the sector from physical zero
	mulu	#128,d2		* index by the length of a sector (128b)
	add.l	#buffer,d2	* d2 now points at the new buffered data
	move.l	cpmdma,d1	* point d1 at the selected dma address
	rts

* move 128 bytes of data from address in a0 to address in a1.
transfer:
	moveq	#31,d2		* the byte count (128 moves)
mov:	move.l	(a0)+,(a1)+	* move 4 bytes
	dbra	d2,mov		* maybe go move some more
	rts

	.ifeq	hard

d2flush:
	clr.l	d0		* set a default of success
	tst.b	dirty		* check the dirty flag
	beq	d2fx		* if the buffer is already available
	move.b	bufdrv,drive	* make the buffered drive current
	move	buftrk,track	* make the buffered cylinder current
	move.b	bufsid,side	* make the buffered head current
	move.b	bufsec,sec	* make the buffered physical sector current
	move.b	#1,hdtemp	* set the command for a write
	bsr	d2rw		* go do the I/O
	clr.b	dirty		* make the buffer available
d2fx:
	rts			* and leave

	.endc
	.ifeq	disk2

* go pre-read a physical sector if it is not already in the buffer
d2buf:
	clr.l	d0		* clear the registers
	clr.l	d7
	clr.l	d6
	clr.l	d5
	clr.l	d4
	move.b	cpmdrv,d7	* get the selected logical major device
	and.b	#$f,d7		* mask it
	move.b	d7,rdrive	* and save it
	move	cpmtrk,d6	* get the selected logical cylinder
	clr.b	rside		* clear the saved head
	move	d6,d5		* and copy it for later use

	.ifeq	d2m20*d2f40b

	and.b	#7,d5		* we have 8 heads in these drives
	lsr	#3,d6		* so we divide by 8 heads for the cylinder

	.endc
	.ifeq	d2m10*d2f20b

	and.b	#3,d5		* we have 4 heads in these drives
	lsr	#2,d6		* se we divide by 4 heads for the cylinder

	.endc

	move	d6,rtrack	* save the physical cylinder
	move.b	d5,rside	* and the physical R/W head
	move.b	cpmsec,d4	* get the selected logical sector
	lsr.b	#3,d4		* divide by 8 sectors per physical sector

	.ifeq	d2f20b*d2f40b

	and.l	#$1f,d4		* we allow up to 32 physical sectors here

	.endc
	.ifeq	d2m10*d2m20

	and.l	#$f,d4		* we allow up to 16 physical sectors here

	.endc

	move.b	d4,rsec		* save the physical sector
	cmp.b	bufdrv,d7
	bne	d2ba		* if the selected major device is resident
	cmp.b	buftrk+1,d6	* if the selected cylinder (one byte) is not
	bne	d2ba		* already resident
	cmp.b	bufsid,d5
	bne	d2ba		* if the selected head is not already resident
	cmp.b	bufsec,d4	* if the selected physical sector is resident
	beq	d2bb		* we bypass the pre-read and continue
d2ba:
	bsr	flush		* flush the dirty buffer, if any
	move.b	rdrive,drive	* make the saved major device new
	move	rtrack,track	* make the save cylinder new
	move.b	rside,side	* make the saved head new
	move.b	rsec,sec	* make the saved physical sector new
	clr.b	hdtemp		* set the command for a read
	bsr	d2rw		* go do the I/O
	move.b	drive,bufdrv	* buffer the new major device
	move	track,buftrk	* buffer the new cylinder
	move.b	side,bufsid	* buffer the new head
	move.b	sec,bufsec	* buffer the new physical sector
	tst.b	d0		* see if the I/O was successful
	bne	d2bc		* if not, we return non-zero to the syscall
d2bb:
	move.b	cpmsec,d2	* get the selected logical sector
	and.l	#7,d2		* mask the least significant 8 sectors/block
	mulu	#128,d2		* multiply by the length of a logical sector
	add.l	#buffer,d2	* index into the sector buffer
	move.l	cpmdma,d1	* set the other pointer to the selected address
d2bc:
	rts

	.endc
	.ifeq	hard

d2rw:
	move.l	#buffer,dma	* point the location at the (only) buffer

	.ifeq	disk3

	move.b	drive,d5	* get the new major device
	and.l	#$f,d5		* mask it
	mulu	#4,d5		* multiply by the length of a pointer
	lea	drvtab,a5	* index into the DPH pointer table
	move.l	0(a5,d5),a5	* get a DPH pointer
	move.l	14(a5),d5	* get a DPB pointer from the DPH
	sub.l	#dpb0,d5	* subtract the first DPB address
	divu	#dpblen,d5	* divide by the length of a DPB
	and.l	#$ff,d5		* mask it just in case
	lea	typbase,a5	* index into the major device type table
	move.b	0(a5,d5),d5	* get the major device type
	cmp.b	#dsk3typ,d5	* is it a DISK3?
	beq	d3rw		* if yes

	.endc
	.endc
	.ifeq	disk2

	move.b	drive,d5	* get the new major device
	and.l	#$f,d5		* mask it
	add.l	#hdtab,d5	* index into the minor device table
	move.l	d5,a5		* point a5 at the minor device
	bsr	d2select	* go see if the drive is available
	bne	d2rwc		* if non-zero we wanted to abort
	move.b	(a5),d2		* get the minor device
	and.l	#3,d2		* mask the unit number
	lea	d2cyl,a0	* point a0 at the current head position table
	move.b	0(a0,d2),d3	* get the current position
	and.l	#$ff,d3		* mask the one byte cylinder
	cmp.b	#$ff,d3		* if all ones we are in uncharted territory
	bne	d2rwa		* if not we continue
	bsr	d2home		* else we move the head to cylinder zero
d2rwa:
	move	track,d4	* get the new cylinder
	cmp.b	d4,d3		* compare it with the old one
	beq	d2rwb		* if equal we continue without seeking
	bsr	d2seek		* else we move the head to a new position
d2rwb:
	bsr	d2xfer		* go do the physical I/O command
	tst.b	d0		* if non-zero we have some error
	beq	d2rwc		* else we quit gracefully
d2rwm:
	clr.l	d0		* clear a register
	lea	d2bmap,a1	* point at the map
	move	#d2scnt-1,d2	* set the map length
	move.b	track+1,d0	* get the cylinder
	lsl.l	#8,d0		* shift it
	move.b	side,d0		* get the head
	lsl.l	#8,d0		* shift it
	move.b	sec,d0		* get the sector
d2bmlp:
	clr.l	d1		* clear another register
	move.b	(a1)+,d1	* get the map cylinder
	lsl.l	#8,d1		* shift it
	move.b	(a1)+,d1	* get the map head
	lsl.l	#8,d1		* shift it
	move.b	(a1)+,d1	* get the map sector
	cmp.l	d0,d1		* compare with the old stuff
	beq	d2rwn		* if a match
	dbra	d2,d2bmlp	* else we try the next map entry
	move.l	#$ff,d0		* we have exhausted the map here
	bra	d2rwp		* so return a failure
d2rwn:
	move.l	a1,d0		* get the map pointer
	sub.l	#d2bmap+3,d0	* decrement by one entry
	divu	#3,d0		* divide by the length of an entry
	move.b	d0,sec		* set it up as the sector
	move	track,d3
	clr	track		* set up for cylinder 0
	clr	d4
	bsr	d2seek		* go move the head
	move.b	#2,side		* select head 2
	bsr	d2xfer		* try to move the data from the new track
	tst.b	d0		* test the transfer for a failure
	beq	d2rwc		* if we have succeeded
d2rwp:
	bsr	d2home		* move the head to cylinder zero
	move	track,d4	* get the new cylinder
	bsr	d2seek		* move the head to it
	bsr	d2xfer		* go try the I/O again
	tst.b	d0		* test for error (non-zero)
	beq	d2rwc		* if no error we are fine
	move.b	#$ff,0(a0,d2)	* else make the head position indeterminate
d2rwc:
	rts

d2select:
	bsr	d2ready		* check the device for ready status
	beq	d2sx		* if ready we return
	bsr	d2ready		* if not we ignore and try again
	beq	d2sx		* return now if ready
	lea	d2msg,a0	* print the state of the device
	bsr	type
d2sel1:
	bsr	constat		* check for a key stroke
	beq	d2sel2		* if none we try to get the device ready again
	bsr	conin		* read the character out of the USART
	bra	d2sx		* and quit
d2sel2:
	bsr	d2ready		* see if the device is ready
	bne	d2sel1		* if not we loop some more
d2sx:
	rts

d2ready:
	move.b	(a5),d0		* get the minor device
	move.b	d0,d1		* copy it for later
	and.b	#$f,d0		* mask the disk unit
	or.b	#d2_strb+d2_rst,d0	* combine with a device select
	move.b	d0,d2ctl	* send it
	nop			* wait 1.5 ms. for the select lines to settle
	nop
	nop
	add.b	#'0'-d2_strb-d2_rst,d0	* add an ASCII bias to the unit number
	move.b	d0,d2msgx	* and put it into the not ready prompt
	and.b	#$f0,d1		* mask the drive select line in the copy
	move.b	d1,d2data	* send it (selecting head zero always)
	move.b	d2stat,d0	* get some status
	and.b	#d2_attn+d2_nrdy,d0	* mask for unit ready
	eor.b	#d2_attn,d0	* flip the attention bit
	rts

* move the selected drive/head to cylinder 0
* entry: a0 points at the cylinder table
*	 d2 = the minor device number
* exit:  d0:0..7 = zero for success
*	 d3 = the new cylinder (always zero)
d2home:
	move.b	d2stat,d1	* get drive status
	and.b	#d2_cyl0,d1	* return now if we are already at cylinder zero
	beq	d2home3
	move.b	d2,d0		* get the minor device
	or.b	#d2_sou+d2_rst,d0	* combine with a step out command
	move.b	d0,d2ctl	* send it
	nop			* wait for 1.5 ms. for the status to settle
	nop
	nop
	clr.b	d4		* set the counter for 256 step pulses
d2home1:
	move.b	d2data,d0	* step the head out by one cylinder
	sub.b	#1,d4		* decrement the counter
	bne	d2home1		* and loop if not done
d2home2:
	move.b	d2stat,d0	* get some status
	and.b	#d2_sekd,d0	* mask the seek complete line
	bne	d2home2		* keep checking until the line goes true
d2home3:
	move.b	#0,0(a0,d2)	* set the current cylinder in the table to zero
	clr.w	d3		* and return zero
	rts

* move the selected drive/head to a new cylinder
* entry: a0 points at the cylinder table
*	 d2 = the minor device number
*	 d3 = the previous cylinder
*	 d4 = the new cylinder
d2seek:
	move.b	#d2_sou,d0	* default to a step out command
	sub.b	d4,d3		* get the difference of the old and the new
	bcc	d2seek0		* if the new was lower than the old cylinder
	move.b	#d2_sin,d0	* else we set up for a step in command
	neg.b	d3		* and take the absolute value of the difference
d2seek0:
	and.l	#$ff,d3		* mask to one byte
	or.b	d2,d0		* or the unit number with the command
	move.b	d0,d2ctl	* and send it
	nop			* wait 1.5 ms. for the lines to settle
	nop
	nop
d2step1:
	move.b	d2data,d0	* step the head by one cylinder
	sub.b	#1,d3		* bump the counter (for buffered seeks only)
	bne	d2step1		* and keep doing this until we are done
d2step2:
	move.b	d2stat,d0	* get some status
	and.b	d2_sekd,d0	* check for a seek complete
	bne	d2step2		* and keep doing this until is it complete
d2skx:
	move.b	d4,0(a0,d2)	* move the new cylinder into the position table
	rts

* move some data
* entry: a5 points at the physical minor device
d2xfer:
	moveq	#retries-1,d7	* set the retry counter
d2xlp:
	move.b	(a5),d1		* get the minor device
	and.b	#3,d1		* mask the disk unit
	or.b	#d2_strb+d2_rst,d1	* combine with a device select command
	move.b	d1,d2ctl	* and send it
	nop			* wait for 1.5 ms. to allow the select
	nop			* lines to settle
	nop
	move.b	(a5),d1		* get the minor device
	and.b	#$f0,d1		* mask the select lines
	or.b	side,d1		* combine with the new head
	move.b	d1,d2data	* and send it
	move.b	#d2_cyl,d2ctl	* sent the select cylinder command
	move.b	track+1,d2data	* and send the new cylinder
	move.b	#d2_head,d2ctl	* send the select head command
	move.b	side,d2data	* and send the new head
	move.b	#d2_sec,d2ctl	* send the select sector command
	move.b	sec,d2data	* and send the new physical sector
	move.b	selchan,d1	* stop the SELECTOR CHANNEL
	move.b	dma+1,selchan	* send the high address byte
	move.b	dma+2,selchan	* send the middle address byte
	move.b	dma+3,selchan	* send the low address byte
	move.b	(a5),d1		* get the minor device
	and.b	#3,d1		* mask the unit number
	tst.b	hdtemp		* check if we read or write
	beq	d2xc		* if we are reading
	move.b	#selbyte,selchan	* move the write to device command
	or.b	#d2_wrt,d1	* combine the unit with the write data command
	bra	d2xd		* go send it
d2xc:
	move.b	#selbyte+$80,selchan	* move the read from device command
	or.b	#d2_read,d1	* combine the unit with the read data command
d2xd:
	move.b	d1,d2ctl	* and send the command
d2xe:
	move.b	d2stat,d0	* go get some status
	blt	d2xe		* wait for the command complete line to go true
	move.b	d2stat,d0	* get the whole status
	and.b	#d2_tout+d2_crc+d2_ovr+d2_nrdy+d2_sekd+d2_wrtf,d0 * mask some
	eor.b	#d2_wrtf,d0	* make the write fault logically positive
	tst.b	d0		* check for problems
	beq	d2xf		* of zero we are okay and can leave nicely
	dbra	d7,d2xlp	* if not loop for a retry
d2xf:
	move.b	#d2_strb,d2ctl	* select device zero to restart the FSM,
	move.b	#$10,d2data	* when we use more than one hard disk 
	rts			* here d0 is non-zero when retries have failed

d2init:
	move.b	#d2base,d1
	bsr	seldsk
	move.l	#d2bmap,cpmdma
	move.w	#0,cpmtrk
	move.b	#d2scnt*8-8,cpmsec
	bsr	read
	rts

	.endc
	.ifeq	disk3

d3buf:
	clr.l	d0		* clear some registers
	clr.l	d7
	clr.l	d6
	clr.l	d5
	clr.l	d4
	move.b	cpmdrv,d7	* get the selected major device
	and.l	#$f,d7		* mask it
	move.b	d7,rdrive	* and save it
	move	cpmtrk,d6	* get the selected logical track
	move	d6,rtrack	* and save it
	move.b	cpmsec,d4	* get the selected logical sector
	lsr.b	#3,d4		* divide by the length of a phsical sector
	move.b	d4,rsec		* and save it
	cmp.b	bufdrv,d7
	bne	d3ba		* if the selected major device is not resident
	cmp	buftrk,d6
	bne	d3ba		* if the selected logical track is not resident
	cmp.b	bufsec,d4
	beq	d3bb		* if the selected physical sector is resident
d3ba:
	bsr	flush		* flush out the old stuff first
	move.b	rdrive,drive	* make the saved major device new
	move	rtrack,track	* make the saved logical track new
	clr.b	side		* make the saved head zero
	move.b	rsec,sec	* make the saved physical sector new
	clr.b	hdtemp		* set up for a read
	bsr	d3rw		* go do the physical pre-read
	move.b	drive,bufdrv	* make the new major device resident
	move	track,buftrk	* make the new track resident
	move.b	side,bufsid	* make the new head (zero) resident
	move.b	sec,bufsec	* make the new sector resident
	tst.b	d0		* check the return status of the read
	bne	d3bc		* if not successful
d3bb:
	move.b	cpmsec,d2	* get the old logical sector
	and.l	#7,d2		* mask it
	mulu	#128,d2		* multiply by the length of a physical sector
	add.l	#buffer,d2	* index into the pre-read buffer
	move.l	cpmdma,d1	* get the user buffer address
d3bc:
	rts

d3rw:
	move.b	drive,d5	* get the major device
	and.l	#$f,d5		* mask it
	lea	hdtab,a5	* point into the minor devce table
	move.b	0(a5,d5),d3	* get a minor device
	and.l	#3,d3		* mask the drive unit number
	lea	d3iopb+2,a2
	move.b	d3,(a2)+	* and save it
	move.b	hdtemp,d0	* go get the direction flag
	eor.b	#1,d0		* invert bit 0
	move.b	d0,(a2)+	* set into the iopb
	move.b	sec,(a2)+	* move the physical sector into the iopb
	move.b	#0,(a2)+	* set the MSB of the iopb sector
	move.b	track+1,(a2)+	* move the MSB of the track into the iopb
	move.b	track+0,(a2)+	* and the LSB of the track
	move	#$100,(a2)+	* set up for a one sector transfer
	move.l	#buffer,d0	* set up the DMA pointer
	move.b	d0,(a2)+
	lsr.l	#8,d0
	move.b	d0,(a2)+
	lsr.l	#8,d0
	move.b	d0,(a2)+
	move.l	#d3_rdwr,d0	* set up the physical read/write command
	bsr	d3exec		* go do a command
	rts			* return whatever we get back from d3exec

d3spec:
	move.l	#defiopb,a0	* point at the initial iopb (0x50)
	lea	d3sav,a1	* point at the save buffer
	move	#15,d0		* the length of an iopb
d3spa:
	move.b	(a0)+,(a1)+	* move a byte
	dbra	d0,d3spa	* maybe move another
	move.l	#d3iopb,d1	* move the next iopb address into the iopb
	move.b	d1,defiopb+13	* and set up the iopb links
	move.b	d1,d3iopb+13
	lsr.l	#8,d1
	move.b	d1,defiopb+14
	move.b	d1,d3iopb+14
	lsr.l	#8,d1
	move.b	d1,defiopb+15
	move.b	d1,d3iopb+15
	move.b	#d3_rst,d3port	* send a reset into the controller
	move.b	#d3_attn,d3port	* now wake it up
	move	#$3ff,d0	* set up to delay about 512 ms
d3spq:
	nop			* NOP uses 500 ns of time in an 8 MHz CPU
	dbra	d0,d3spq	* we wait here for the DISK3 to catch up

	move.b	#d3_nop,d3iopb+0 * do a NOOP to check for a controller
	move.l	#$ffff,d0	* set the timeout counter
	clr.b	d3iopb+1	* set the iopb busy
	move.b	#d3_attn,d3port
d3spx:
	tst.b	d3iopb+1	* get some iopb status
	bne	d3spd		* if yes
	sub.l	#1,d0		* bump the timeout counter
	bne	d3spx		* maybe try again
	lea	d3msg2,a0	* point at the message
	bsr	type		* and print it
	move.l	#$ff,d0
	bra	d3spz		* restore the saved iopb and quit early
d3spd:
	move.l	#d3_home,d0
	bsr	d3exec		* do a recalibrate now
	move.b	#1,d3iopb+3	* we are going to read something now
	clr.l	d3iopb+4	* we are reading from cylinder 0, head 0
	move	#$200,d3iopb+8	* and we are reading 2 sectors
	move.l	#buffer,d0	* point at the pre-read buffer
	lea	d3iopb+10,a0	* and point at the data field of the iopb
	move.b	d0,(a0)+	* place the buffer address in the data field
	lsr.l	#8,d0
	move.b	d0,(a0)+
	lsr.l	#8,d0
	move.b	d0,(a0)+
	move.l	#d3_rdwr,d0
	bsr	d3exec		* go read the sectors into the pre-read buffer
	move.l	buffer,d0	* get the first four bytes from the buffer
	cmp.l	#'Comp',d0	* check for the marker
	beq	d3spc		* if found we continue
	move.b	d3iopb+2,d0	* else we set the message
	add.b	#'0',d0		* ASCII bias
	move.b	d0,d3msgx	* set the number in the message
	lea	d3msg,a0	* point at the message
	bsr	type		* print it
	move.l	#$ff,d0		* set up for a failure
	bra	d3spz		* and quit now
d3spc:
	move.l	#buffer+16,d1	* point at the specify block
	lea	d3iopb+10,a0	* and point at the data field of the iopb
	move.b	d1,(a0)+	* place the specify block in the data field
	lsr.l	#8,d1
	move.b	d1,(a0)+
	lsr.l	#8,d1
	move.b	d1,(a0)+
	move.l	#d3_spec,d0	* go do a specify command
	bsr	d3exec
	move.l	#buffer+1024,d0	* point at the map block
	lea	d3iopb+10,a0	* and point at the data field of the iopb
	move.b	d0,(a0)+	* place the buffer address in the data field
	lsr.l	#8,d0
	move.b	d0,(a0)+
	lsr.l	#8,d0
	move.b	d0,(a0)+
	move.l	#d3_map,d0	* go do a set map command
	bsr	d3exec
	move.b	d3iopb+1,d0	* get the status
	cmp.b	#d3_cmpl,d0	* check for success
	bne	d3spz		* if not we have non-zero in d0
	clr.l	d0		* else we return success
d3spz:
	move.l	#defiopb,a0	* point at the initial iopb
	lea	d3sav,a1	* and the save buffer
	move	#15,d1		* the length of an iopb
d3spb:
	move.b	(a1)+,(a0)+	* restore a byte from the save buffer
	dbra	d1,d3spb	* maybe go do another
	rts

d3exec:
	move.b	d0,d3iopb	* put the command into the iopb
	clr.b	d3iopb+1	* clear the status field of the iopb
	move.b	#d3_attn,d3port	* get the drive's attention
d3xa:
	move.b	d3iopb+1,d0	* get the status field
	tst.b	d0		* if it is still zero we wait some more
	beq	d3xa
	cmp.b	#d3_cmpl,d0	* check the status for success (0xff)
	bne	d3xf		* if not
	clr.l	d0		* here we return success (zero)
d3xf:
	rts			* here any non-zero in d0 marks a failure

	.endc
	.ifeq	hmdrive

* M-Drive/H Support
* vread and vwrite perform reads and writes on the ram virtual disk

vread:
	bsr	setadr		* set the DMA address
	move.l	cpmdma,a1	* get the destination pointer
	move	#127,d0		* move 128 bytes
	move.b	#$5a,d1		* the checksum bias
vrl:
	move.b	mddata,d2	* get a data byte
	add.b	d2,d1		* add a copy of it to the checksum
	move.b	d2,(a1)+	* and put it into the user buffer
	dbra	d0,vrl		* maybe go back for more
	move.b	cpmsec,mdadr	* the logcal sector forms the MSB
	move.b	cpmtrk+0,d0	* track bit 8 forms the middle
	and.b	#1,d0		* send it (for a track address of 0..511)
	move.b	d0,mdadr
	move.b	cpmtrk+1,mdadr	* track bis 0..7 forms the LSB of the checkadr
	move.b	mddata,d0	* get the expected checksum
	sub.b	d1,d0		* clear d0 if the expected and computed
	bne	vre		* checksums are the same
	clr.l	d0
vre:
	rts

vwrite:
	bsr	setadr		* set the DMA address
	move.l	cpmdma,a1	* point at the user buffer
	move	#127,d0		* move 128 bytes
	move	#$5a,d1		* get the checksum bias
vwl:
	move.b	(a1)+,d2	* get the user data byte
	add.b	d2,d1		* add a copy to the checksum
	move.b	d2,mddata	* send it
	dbra	d0,vwl		* maybe go back for more
	move.b	cpmsec,mdadr	* the logical sector forms the MSB
	move.b	cpmtrk+0,d0	* track bit 8 forms the middle
	and.b	#1,d0		* of the checkadr in a range of 0..511
	move.b	d0,mdadr	* send it
	move.b	cpmtrk+1,mdadr	* send track bits 0..7 as the LSB
	move.b	d1,mddata	* send the computed checksum
	clr.l	d0		* always return success
	rts

setadr:
	move.b	cpmsec,mdadr	* send the secto as the MSB
	move.w	cpmtrk,d6	* get the track
	move.w	d6,d7		* copy it
	lsr.w	#1,d6		* send the middle and LSB of the address
	move.b	d6,mdadr	* by shifting left the track by 7 bits
	lsl.w	#7,d7		* the remaining 7 bits of the address
	move.b	d7,mdadr	* form a 128 byte counter in the M-DRIVE
	rts

setsize:
	lea	dpbm,a0		* point at the memory disk DPB
	move	#0,cpmtrk	* set the track to 0
	move.b	#7,cpmsec	* we offset into the track by 7 sectors each
set1:
	bsr	checksize	* go see if some memory is there
	bne	set2		* if it is not
	move.b	cpmsec,d0	* get the current sector
	add.b	#8,d0		* add 8 sectors to point at the next board
	move.b	d0,cpmsec	* (adds 1K to the board pointer)
	cmp.b	#64,d0		* if we have already tested 8 boards
	blo	set1		* else we go try another
set2:
	moveq	#0,d0		* clear a register
	move.b	cpmsec,d0	* get the board counter
	sub	#7,d0		* point it at the last possible sector
	move	d0,(a0)		* place it in the SPT field of the DPB
	lsr	#2,d0		* divide by 4
	mulu	#127,d0		* multiply by the length of a directory sector
	sub	#1,d0		* (a weird number)
	move	d0,6(a0)	* place it in the DSM field of the DPB
	cmp	#1024,d0	* if less than 1024 directory entries
	bcs	setx		* then we are done with DRM of 0c000h
	move	#255,8(a0)	* else set the directory count to 256
	move	#$f000,10(a0)	* and set the DRM field to 0f000h
setx:
	rts

checksize:
	bsr	setadr		* set the address in the memory disk
	move.b	mddata,d0	* get a data byte
	move	d0,d1		* copy it
	not.b	d0		* one's complement
	bsr	setadr		* point at the same byte as before
	move.b	d0,mddata	* send the complemented byte
	bsr	setadr		* point at the same byte again
	move.b	mddata,d0	* get the data
	eor.b	d1,d0		* XOR the normal with the complemented data
	cmp.b	#$ff,d0		* should be zero if memory was there
	bne	checkx		* if not we return failure
	bsr	setadr		* point at the same old byte
	move.b	d1,mddata	* restore old data
	moveq	#0,d0		* setup to return success
checkx:
	rts

	.endc

	.data

	.even

cpmiob:	.dc.b	0		* the I/O byte
cpmflg:	.dc.b	0		* the selected major device DPB type
cpmtyp:	.dc.b	0		* the saved write command type
cpmdma:	.dc.l	0		* the selected dma address
dma:	.dc.l	0		* the current dma address
cpmdrv:	.dc.b	0		* the selected major device
cpmsec:	.dc.b	0		* the selected logical sector
track:	.dc.w	0		* the current cylinder
cpmtrk:	.dc.w	0		* the selected cylinder
buftrk:	.dc.w	$99		* the buffered cylinder
rtrack:	.dc.w	0		* the new cylinder
bufdrv:	.dc.b	$99		* the buffered major device
bufsid:	.dc.b	0		* the buffered head
bufsec:	.dc.b	0		* the buffered logical sector
rdrive:	.dc.b	0		* the new major device
rside:	.dc.b	0		* the new head
rsec:	.dc.b	0		* the new logical sector
drive:	.dc.b	0		* the current major device
side:	.dc.b	0		* the current head
sec:	.dc.b	0		* the current logical sector
dirty:	.dc.b	0		* the dirty buffer flag byte

	.even

news:	.dc.b	0,0,0,0		* new drive flag for each drive
dens:	.dc.b	0,0,0,0		* current density for each drive
sides:	.dc.b	0,0,0,0		* sidedness flag for each drive

region:	.dc.w	1		* one region
	.dc.l	lomem		* start of TPA
	.dc.l	himem-lomem	* length

signon:	.dc.b	k_cls,'CompuPro CP/M-68K version '
	.dc.b	model+'0','.',make+'0',patch+'@',k_cr,k_lf
	.dc.b	'Copyright (c) 1983 CompuPro',k_cr,k_lf,0

nrerr:	.dc.b	' Not ready',0
wperr:	.dc.b	' Write protected',0
drverr:	.dc.b	' on drive ',0
qmess:	.dc.b	' Type R to retry, A to abort ',0

	.ifeq	hard

hdtemp:	.dc.b	0		* saved r/w status for hard disk I/O

	.endc
	.ifeq	disk2

d2cyl:	.dc.l	$ffffffff	* the current cylinders for each minor device
d2msg:	.dc.b	'DISK2 unit '
d2msgx:	.dc.b	'x is not ready...',0
d2rhbf:	.dc.b	0,0,0		* for the disk header info

	.endc
	.ifeq	disk3

d3iopb:	.dc.l	0,0,0,0		* the working IOPB
d3sav:	.dc.l	0,0,0,0		* the save block for initialization

d3msg:	.dc.b	'DISK3 unit '
d3msgx:	.dc.b	'x is not formatted',k_cr,k_lf,0
d3msg2:	.dc.b	'DISK3 controller does not respond',k_cr,k_lf,0

	.endc

* disk parameter headers

	.even

dph0:	.dc.l	xlt0		* XLT table address
	.dc.w	0,0,0		* dummy
	.dc.l	dirbuf		* directory buffer pointer
	.dc.l	dpb0		* DPB address
	.dc.l	ckv0		* CKV address
	.dc.l	alv0		* ALV address

dph1:	.dc.l	xlt0		* XLT table address
	.dc.w	0,0,0		* dummy
	.dc.l	dirbuf		* directory buffer pointer
	.dc.l	dpb0		* DPB address
	.dc.l	ckv1		* CKV address
	.dc.l	alv1		* ALV address

	.ifeq	xd8

dph2:	.dc.l	xlt0		* XLT table address
	.dc.w	0,0,0		* dummy
	.dc.l	dirbuf		* directory buffer pointer
	.dc.l	dpb0		* DPB pointer
	.dc.l	ckv2		* CKV pointer
	.dc.l	alv2		* ALV pointer

dph3:	.dc.l	xlt0		* XLT table address
	.dc.w	0,0,0		* dummy
	.dc.l	dirbuf		* directory buffer pointer
	.dc.l	dpb0		* DPB address
	.dc.l	ckv3		* CKV address
	.dc.l	alv3		* ALV address

	.endc
	.ifeq	disk2

dphd2a:	.dc.l	0		* XLT table address
	.dc.w	0,0,0		* dummy
	.dc.l	dirbuf		* directory buffer pointer
	.dc.l	dpbd2a		* DPB address
	.dc.l	0		* CKV address
	.dc.l	alvd2a		* ALV address

dphd2b:	.dc.l	0		* XLT table address
	.dc.w	0,0,0		* dummy
	.dc.l	dirbuf		* directory buffer pointer
	.dc.l	dpbd2b		* DPB address
	.dc.l	0		* CKV address
	.dc.l	alvd2b		* ALV address

	.endc
	.ifeq	d2m20*d2f20b*d2f40b

dphd2c:	.dc.l	0		* XLT table address
	.dc.w	0,0,0		* dummy
	.dc.l	dirbuf		* directory buffer address
	.dc.l	dpbd2c		* DPB address
	.dc.l	0		* CKV address
	.dc.l	alvd2c		* ALV addres

	.endc
	.ifeq	d2f40b

dphd2d:	.dc.l	0		* XLT table address
	.dc.w	0,0,0		* dummy
	.dc.l	dirbuf		* directory buffer pointer
	.dc.l	dpbd2d		* DPB address
	.dc.l	0		* CKV address
	.dc.l	alvd2d		* ALV address

dphd2e:	.dc.l	0		* XLT table address
	.dc.w	0,0,0		* dummy
	.dc.l	dirbuf		* directory buffer pointer
	.dc.l	dpbd2e		* DPB address
	.dc.l	0		* CKV address
	.dc.l	alvd2e		* ALV address

	.endc
	.ifeq	disk3

dphd3a:	.dc.l	0		* XLT table address
	.dc.w	0,0,0		* dummy
	.dc.l	dirbuf		* directory buffer pointer
	.dc.l	dpbd3a		* DPB address
	.dc.l	0		* CKV address
	.dc.l	alvd3a		* ALV address

	.endc
	.ifeq	d3m40

dphd3b:	.dc.l	0		* XLT table address
	.dc.w	0,0,0		* dummy
	.dc.l	dirbuf		* directory buffer pointer
	.dc.l	dpbd3b		* DPB address
	.dc.l	0		* CKV address
	.dc.l	alvd3b		* ALV address

dphd3c:	.dc.l	0		* XLT table address
	.dc.w	0,0,0		* dummy
	.dc.l	dirbuf		* directory buffer pointer
	.dc.l	dpbd3c		* DPB address
	.dc.l	0		* CKV address
	.dc.l	alvd3c		* ALV address

dphd3d:	.dc.l	0		* XLT table address
	.dc.w	0,0,0		* dummy
	.dc.l	dirbuf		* directory buffer pointer
	.dc.l	dpbd3d		* DPB address
	.dc.l	0		* CKV address
	.dc.l	alvd3d		* ALV address

dphd3e:	.dc.l	0		* XLT table address
	.dc.w	0,0,0		* dummy
	.dc.l	dirbuf		* directory buffer pointer
	.dc.l	dpbd3e		* DPB address
	.dc.l	0		* CKV address
	.dc.l	alvd3e		* ALV address

	.endc
	.ifeq	hmdrive

dphm:	.dc.l	0		* XLT table address
	.dc.w	0,0,0		* dummy
	.dc.l	dirbuf		* directory buffer pointer
	.dc.l	dpbm		* DPB address
	.dc.l	0		* CKV address
	.dc.l	alvm		* ALV address

	.endc

	.even

* disk parameter blocks:
* the CompuPro CBIOS requires the first 8 DPB's be for the floppy disk only

dpb0:	.dc.w	26		* sectors/track
	.dc.b	3		* block shift
	.dc.b	7		* block mask
	.dc.b	0		* extent mask
	.dc.b	0		* dummy
	.dc.w	242		* disk size
	.dc.w	63		* directory entries
	.dc.w	$c000		* directory mask
	.dc.w	16		* directory check size
	.dc.w	2		* track offset

dpb0d:	.dc.w	26		* sectors/track
	.dc.b	4		* block shift
	.dc.b	15		* block mask
	.dc.b	1		* extent mask
	.dc.b	0		* dummy
	.dc.w	242		* disk size
	.dc.w	127		* directory entries
	.dc.w	$c000		* directory mask
	.dc.w	32		* directory check size
	.dc.w	4		* track offset

dpb1:	.dc.w	52		* sectors/track
	.dc.b	4		* block shift
	.dc.b	15		* block mask
	.dc.b	0		* extent mask
	.dc.b	0		* dummy
	.dc.w	242		* disk size
	.dc.w	127		* directory entries
	.dc.w	$c000		* directory mask
	.dc.w	32		* directory check size
	.dc.w	2		* track offset

dpb1d:	.dc.w	52		* sectors/track
	.dc.b	4		* block shift
	.dc.b	15		* block mask
	.dc.b	0		* extent mask
	.dc.b	0		* dummy
	.dc.w	485		* disk size
	.dc.w	255		* directory entries
	.dc.w	$f000		* directory mask
	.dc.w	64		* directory check size
	.dc.w	4		* track offset

dpb2:	.dc.w	60		* sectors/track
	.dc.b	4		* block shift
	.dc.b	15		* block mask
	.dc.b	0		* extent mask
	.dc.b	0		* dummy
	.dc.w	261		* disk size
	.dc.w	127		* directory entries
	.dc.w	$c000		* directory mask
	.dc.w	32		* directory check size
	.dc.w	2		* track offset

dpb2d:	.dc.w	60		* sectors/track
	.dc.b	4		* block shift
	.dc.b	15		* block mask
	.dc.b	0		* extent mask
	.dc.b	0		* dummy
	.dc.w	523		* disk size
	.dc.w	255		* directory entries
	.dc.w	$f000		* directory mask
	.dc.w	64		* directory check size
	.dc.w	4		* track offset

dpb3:	.dc.w	64		* sectors/track
	.dc.b	4		* block shift
	.dc.b	15		* block mask
	.dc.b	0		* extent mask
	.dc.b	0		* dummy
	.dc.w	299		* disk size
	.dc.w	127		* directory entries
	.dc.w	$c000		* directory mask
	.dc.w	32		* directory check size
	.dc.w	2		* track offset

dpb3d:	.dc.w	64		* sectors/track
	.dc.b	4		* block shift
	.dc.b	15		* block mask
	.dc.b	0		* extent mask
	.dc.b	0		* dummy
	.dc.w	599		* disk size
	.dc.w	255		* directory entries
	.dc.w	$f000		* directory mask
	.dc.w	64		* directory check size
	.dc.w	4		* track offset

	.ifeq	d2m10

dpbd2a:	.dc.w	d2scnt*8	* sectors/track
	.dc.b	5		* block shift
	.dc.b	31		* block mask
	.dc.b	1		* extent mask
	.dc.b	0		* dummy
	.dc.w	1342		* disk size
	.dc.w	1023		* directory entries
	.dc.w	$ffff		* directory mask
	.dc.w	0		* directory check size
	.dc.w	1*4		* track offset

dpbd2b:	.dc.w	d2scnt*8	* sectors/track
	.dc.b	5		* block shift
	.dc.b	31		* block mask
	.dc.b	1		* extent mask
	.dc.b	0		* dummy
	.dc.w	1331		* disk size
	.dc.w	1023		* directory entries
	.dc.w	0		* directory check size
	.dc.w	123*4		* track offset

	.endc
	.ifeq	d2m20*d2f20b

dpbd2a:	.dc.w	d2scnt*8	* sectors/track
	.dc.b	5		* block shift
	.dc.b	31		* block mask
	.dc.b	1		* extent mask
	.dc.b	0		* dummy
	.dc.w	2045		* disk size
	.dc.w	1023		* directory entries
	.dc.w	$ffff		* directory mask
	.dc.w	0		* directory check size
	.dc.w	8*1		* track offset

dpbd2b:	.dc.w	d2scnt*8	* sectors/track
	.dc.b	5		* block shift
	.dc.b	31		* block mask
	.dc.b	1		* extent mask
	.dc.b	0		* dummy
	.dc.w	2045		* disk size
	.dc.w	1023		* directory entries
	.dc.w	$ffff		* directory mask
	.dc.w	0		* directory check size
	.dc.w	8*94		* track offset

dpbd2c:	.dc.w	d2scnt*8	* sectors/track
	.dc.b	5		* block shift
	.dc.b	31		* block mask
	.dc.b	1		* extent mask
	.dc.b	0		* dummy
	.dc.w	1231		* disk size
	.dc.w	1023		* directory entries
	.dc.w	$ffff		* directory mask
	.dc.w	0		* directory check size
	.dc.w	8*187		* track offset

	.endc
	.ifeq	d2f40b

dpbd2a:	.dc.w	d2scnt*8	* sectors/track
	.dc.b	5		* block shift
	.dc.b	31		* block mask
	.dc.b	1		* extent mask
	.dc.b	0		* dummy
	.dc.w	2023		* disk size
	.dc.w	1023		* directory entries
	.dc.w	$ffff		* directory mask
	.dc.w	0		* directory check size
	.dc.w	8*1		* track offset

dpbd2b:	.dc.w	d2scnt*8	* sectors/track
	.dc.b	5		* block shift
	.dc.b	31		* block mask
	.dc.b	1		* extent mask
	.dc.b	0		* dummy
	.dc.w	2023		* disk size
	.dc.w	1023		* directory entries
	.dc.w	$ffff		* directory mask
	.dc.w	0		* directory check size
	.dc.w	8*47		* track offset

dpbd2c:
	.dc.w	d2scnt*8	* sectors/track
	.dc.b	5		* block shift
	.dc.b	31		* block mask
	.dc.b	1		* extent mask
	.dc.b	0		* dummy
	.dc.w	2023		* disk size
	.dc.w	1023		* directory entries
	.dc.w	$ffff		* directory mask
	.dc.w	0		* directory check size
	.dc.w	8*93		* track offset

dpbd2d:	.dc.w	d2scnt*8	* sectors/track
	.dc.b	5		* block shift
	.dc.b	31		* block mask
	.dc.b	1		* extent mask
	.dc.b	0		* dummy
	.dc.w	2023		* disk size
	.dc.w	1023		* directory entries
	.dc.w	$ffff		* directory mask
	.dc.w	0		* directory check size
	.dc.w	8*139		* track offset

dpbd2e:	.dc.w	d2scnt*8	* sectors/track
	.dc.b	5		* block shift
	.dc.b	31		* block mask
	.dc.b	1		* extent mask
	.dc.b	0		* dummy
	.dc.w	2463		* disk size
	.dc.w	1023		* directory entres
	.dc.w	$ffff		* directory mask
	.dc.w	0		* directory check size
	.dc.w	8*185		* track offset

	.endc
	.ifeq	disk3

dpbd3a:	.dc.w	d3scnt*8	* sectors/track
	.dc.b	5		* block shift
	.dc.b	31		* block mask
	.dc.b	1		* extent mask
	.dc.b	0		* dummy
	.dc.w	1331		* disk size
	.dc.w	1023		* directory entries
	.dc.w	$ffff		* directory mask
	.dc.w	0		* directory check size
	.dc.w	2		* track offset

	.endc
	.ifeq	d3m40

dpbd3b:	.dc.w	d3scnt*8	* sectors/track
	.dc.b	5		* block shift
	.dc.b	31		* block mask
	.dc.b	1		* extent mask
	.dc.b	0		* dummy
	.dc.w	2519		* disk size
	.dc.w	1023		* directory entries
	.dc.w	$ffff		* directory mask
	.dc.w	0		* drectory check size
	.dc.w	8*74		* track offset

dpbd3c:
	.dc.w	d3scnt*8	* sectors/track
	.dc.b	5		* block shift
	.dc.b	31		* block mask
	.dc.b	1		* extent mask
	.dc.b	0		* dummy
	.dc.w	2519		* disk size
	.dc.w	1023		* directory entries
	.dc.w	$ffff		* directory mask
	.dc.w	0		* directory check size
	.dc.w	8*211		* track offset

dpbd3d:	.dc.w	d3scnt*8	* sectors/track
	.dc.b	5		* block shift
	.dc.b	31		* block mask
	.dc.b	1		* extent mask
	.dc.b	0		* dummy
	.dc.w	2519		* disk size
	.dc.w	1023		* directory entries
	.dc.w	$ffff		* directory mask
	.dc.w	0		* directory check size
	.dc.w	8*351		* track offset

dpbd3e:	.dc.w	d3scnt*8	* sectors/track
	.dc.b	4		* block shift
	.dc.b	15		* block mask
	.dc.b	0		* extent mask
	.dc.b	0		* dummy
	.dc.w	611		* disk size
	.dc.w	255		* directoy entres
	.dc.w	$f000		* directory mask
	.dc.w	0		* directory check size
	.dc.w	8*491		* track offset

	.endc
	.ifeq	hmdrive

dpbm:	.dc.w	8		* sectors/track
	.dc.b	4		* block shift
	.dc.b	15		* block mask
	.dc.b	0		* extent mask
	.dc.b	0		* dummy
	.dc.w	253		* disk size
	.dc.w	127		* directory entries
	.dc.w	$c000		* directory mask
	.dc.w	0		* directory check size
	.dc.w	4		* track offset

	.endc

	.even

typbase:
	.dc.b	fp8s1d0,fp8s2d0,fp8s1d1,fp8s2d1		* for the floppies
	.dc.b	fp8s1d2,fp8s2d2,fp8s1d3,fp8s2d3

	.ifeq	disk2

	.dc.b	dsk2typ,dsk2typ				* for any DISK2

	.endc
	.ifeq	d2m20*d2f20b*d2f40b

	.dc.b	dsk2typ					* if not d2m10

	.endc
	.ifeq	d2f40b

	.dc.b	dsk2typ,dsk2typ				* if d2f40b

	.endc
	.ifeq	disk3

	.dc.b	dsk3typ					* for any DISK3

	.endc
	.ifeq	d3m40

	.dc.b	dsk3typ,dsk3typ,dsk3typ,dsk3typ		* if Q540

	.endc
	.ifeq	hmdrive

	.dc.b	memtype					* for the memory disk

	.endc

	.even

	.ifeq	hard

hdtab:	.dc.b	0,0,0,0		* drives A-D: no hard disk
	.dc.b	$10		* drive E: hard disk unit 0

	.ifeq	disk2*d3m40

	.dc.b	$10		* drive F: hard disk unit 0

	.endc
	.ifeq	d2m20*d2f20b*d2f40b*d3m40

	.dc.b	$10		* drive G: hard disk unit 0

	.endc
	.ifeq	d2f40b*d3m40

	.dc.b	$10,$10		* drives H-I: hard disk unit 0

	.endc
	.ifne	disk2*d3m40

	.dc.b	0		* drive F: no hard disk

	.endc
	.ifne	d2m20*d2f20b*d2f40b*d3m40

	.dc.b	0		* drive G: no hard disk

	.endc
	.ifne	d2f40b*d3m40

	.dc.b	0,0		* drives H-I: no hard disk

	.endc

	.dc.b	0,0,0,0,0,0,0	* drives J-P: no hard disk

	.endc

* active drive table
drvtab:
	.dc.l	dph0,dph1	* drive A: 8" floppy disk unit 0

	.ifeq	xd8

	.dc.l	dph2,dph3	* drive C: 8" floppy disk unit 2

	.endc
	.ifne	xd8

	.dc.l	0,0		* drive C:

	.endc
	.ifeq	disk2

	.dc.l	dphd2a,dphd2b	* drive E: hard disk unit 0

	.ifeq	d2m20*d2f20b*d2f40b

	.dc.l	dphd2c		* drive G: hard disk unit 2

	.endc
	.ifeq	d2f40b

	.dc.l	dphd2d,dphd2e	* drives H-I: hard disk unit 3

	.endc
	.ifne	d2m20*d2f20b*d2f40b

	.dc.l	0		* drive G:

	.endc
	.ifne	d2f40b

	.dc.l	0,0		* drives H-I:

	.endc
	.endc
	.ifeq	disk3

	.dc.l	dphd3a		* drive E: hard disk unit 0

	.ifeq	d3m40

	.dc.l	dphd3b,dphd3c	* drive F: hard disk unit 1
	.dc.l	dphd3d,dphd3e	* drive H: hard disk unit 3

	.endc
	.ifne	disk3

	.dc.l	0		* drive E:

	.endc
	.ifne	d3m40

	.dc.l	0,0,0,0		* drive F:

	.endc
	.endc
	.ifne	hard

	.dc.l	0,0,0,0,0	* drive E:

	.endc

	.dc.l	0,0,0		* drive J:

	.ifeq	hmdrive

	.dc.l	dphm		* drive M: M-DRIVE/H memory drive

	.endc
	.ifne	hmdrive

	.dc.l	0		* drive M:

	.endc

	.dc.l	0,0,0		* drive N:

	.even

* sector translation tables

xlt0:	.dc.b	1, 7,13,19,25, 5,11,17,23, 3, 9,15,21
	.dc.b	2, 8,14,20,26, 6,12,18,24, 4,10,16,22

xlt1:	.dc.b	1, 2, 19,20, 37,38,  3, 4, 21,22, 39,40
	.dc.b	5, 6, 23,24, 41,42,  7, 8, 25,26, 43,44
	.dc.b	9,10, 27,28, 45,46, 11,12, 29,30, 47,48
	.dc.b  13,14, 31,32, 49,50, 15,16, 33,34, 51,52
	.dc.b  17,18, 35,36

xlt2:	.dc.b	1, 2, 3, 4, 17,18,19,20, 33,34,35,36, 49,50,51,52
	.dc.b	5, 6, 7, 8, 21,22,23,24, 37,38,39,40, 53,54,55,56
	.dc.b	9,10,11,12, 25,26,27,28, 41,42,43,44, 57,58,59,60
	.dc.b  13,14,15,16, 29,30,31,32, 45,46,47,48

xlt3:	.dc.b   1, 2, 3, 4, 5, 6, 7, 8, 25,26,27,28,29,30,31,32
	.dc.b  49,50,51,52,53,54,55,56,  9,10,11,12,13,14,15,16
	.dc.b  33,34,35,36,37,38,39,40, 57,58,59,60,61,62,63,64
	.dc.b  17,18,19,20,21,22,23,24, 41,42,43,44,45,46,47,48

xlts:	.dc.l  xlt0,  xlt1, xlt2,  xlt3

gaps:	.dc.b	26,$07,$80	* density 0
	.dc.b	26,$0e,$ff	* density 1
	.dc.b	15,$1b,$ff	* density 2
	.dc.b	08,$35,$ff	* density 3

	.bss

	.ifeq	disk2

d2bmap:	.ds.b	128		* the sector relocation map

	.endc

cmd:    .ds.b   10		* FDC command table
stat:   .ds.b   8		* FDC result buffer

ckv0:	.ds.b	64		* FDC device 0 CKV
ckv1:	.ds.b	64		* FDC device 1 CKV

	.ifeq	xd8

ckv2:	.ds.b	64		* FDC device 2 CKV
ckv3:	.ds.b	64		* FDC device 3 CKV

	.endc

alv0:	.ds.b	80		* FDC device 0 ALV
alv1:	.ds.b	80		* FDC device 1 ALV

	.ifeq	xd8

alv2:	.ds.b	80		* FDC device 2 ALV
alv3:	.ds.b	80		* FDC device 3 ALV

	.endc
	.ifeq	disk2

alvd2a:	.ds.b	260		* DISK2 device 0 ALV
alvd2b:	.ds.b	260		* DISK2 device 1 ALV

	.endc
	.ifeq	d2m20*d2f20b*d2f40b

alvd2c:	.ds.b	256		* DISK2 device 2 ALV

	.endc
	.ifeq	d2f40b

alvd2d:	.ds.b	256		* DISK2 device 3 ALV
alvd2e:	.ds.b	310		* DISK2 device 4 ALV

	.endc
	.ifeq	disk3

alvd3a:	.ds.b	170		* DISK3 device 0 ALV

	.endc
	.ifeq	d3m40

alvd3b:	.ds.b	320		* DISK3 device 1 ALV
alvd3c:	.ds.b	320		* DISK3 device 2 ALV
alvd3d:	.ds.b	320		* DISK3 device 3 ALV
alvd3e:	.ds.b	80		* DISK3 device 4 ALV

	.endc
	.ifeq	hmdrive

alvm:	.ds.b	64		* M-DRIVE/H ALV

	.endc

dirbuf:	.ds.b	128		* file system directory update buffer

buffer:	.ds.b	$2000		* FDC blocking/deblocking buffer

	.end
