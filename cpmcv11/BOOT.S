*****************************************************************
*								*
*	CP/M-68K Boot Loader					*
*	for CompuPro CPU-68K and Disk 1 floppy disk controller	*
*	by Michael A. Perry					*
*	last modified 08May83					*
*	Double Density						*
*								*
*****************************************************************


iobase	=	$ff0000		* base of 68000's memory mapped i/o

dstat	=	iobase+$c0	* disk status port
ddata	=	dstat+1		* disk data port
ddma	=	dstat+2		* disk dma port

retries	=	3

start:
	lea	$10000,a7
	lea	dstat,a0
	lea	ddata,a1
	lea	ddma,a2
* set dma address for data transfer
        move.b  #$00,(a2)
        move.b  #$01,(a2)
        move.b  #$00,(a2)
* home the disk
        moveq   #2,d2		* 3 retries
rest1:
        moveq   #$7,d4
        bsr     send
        move.b  #0,d4
        bsr     send
        bsr     sis		* get status
        dbeq    d2,rest1	* retry on error
* read track zero
	move.l	#cmd,a3
        bsr     command		* issue command
	move	stat,d4		* was disk i/o successful?
	sub	#$4080,d4	* 4080 is status for good read or write
	bne	start
* position the disk head to track 1
        moveq   #2,d2		* 3 retries
seek1:
        move.b	#$0f,d4
        bsr     send
        move.b  #0,d4
        bsr     send
        move.b  #1,d4		* track number
        bsr     send
        bsr     sis		* get status
	dbeq	d2,seek1	* retry on error
* Read ID field: find density
	move.b	#$4a,d4
	bsr	send
	move.b	#0,d4
	bsr	send
	bsr	waitint
	bsr	results		* read all result bytes
	moveq	#0,d3
	move	stat,d2
	bne	read		* bad status means single density, else
	move.b	stat+6,d3	* get density byte
read:
* routine to read track one
* select the command buffer for the next read
	mulu	#9,d3		* index into cmd table
	add.l	#cmds,d3
	move.l	d3,a3		* point a3 at command buffer
        bsr     command		* issue command
	move.w	stat,d4		* was disk i/o successful?
	and	#$f8ff,d4	* ignore drive number
	sub	#$4080,d4	* 4080 is status for good read or write
	bne	start
	bra	loader

send:
* send byte in d4 to disk controller
        btst    #7,(a0)
        beq     send
        move.b  d4,(a1)
        rts

results:
* read all result bytes from disk controller
        lea   stat,a3
res1:
        btst    #7,(a0)
        beq     res1
        btst    #6,(a0)
        beq     resx
        move.b  (a1),(a3)+
        bra     res1
resx:   rts

waitint:
* wait until Disk 1 interrupt flag becomes true
	btst	#7,(a2)
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

command:
* send 9 bytes from command buffer to disk controller
        moveq   #8,d3		* send 9 bytes
com:
        move.b  (a3)+,d4
        bsr     send
        dbra    d3,com
	bsr	waitint		* wait until done
        bsr	results		* read all result bytes
	rts

cmd:	.dc.b	6,0,0,0,3,0,26,7,128
cmds:
cmd0:	.dc.b	$06,0,1,0,1,0,26,$07,$80
cmd1:	.dc.b	$46,0,1,0,1,1,26,$0e,$ff
cmd2:	.dc.b	$46,0,1,0,1,2,15,$1b,$ff
cmd3:	.dc.b	$46,0,1,0,1,3,08,$35,$ff

	.even
stat:   .ds.b   8

	.even
loader:
	nop
	.end
